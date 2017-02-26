/*
 * This file is part of uGB
 * Copyright (C) 2017  Alexandre Monti
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define _POSIX_SOURCE

#include "debugger.h"
#include "cpu.h"
#include "mmu.h"
#include "opcodes.h"
#include "errno.h"

#include <stdlib.h>
#include <stdio.h>
#include <setjmp.h>
#include <signal.h>
#include <readline/readline.h>
#include <readline/history.h>

static int _breakpoint_id = 1;
static ugb_breakpoint* _breakpoints = 0;
static ugb_breakpoint* _last_breakpoint = 0;

static char _quit;
static ugb_gbm* _gbm;
static sigjmp_buf _jmpbuf;

static char* _skip_whitespace(char* line);
static char* _strip_whitespace(char* line);
static void _handle_signals(int signo);
static void _com_help(char* args);
static void _com_quit(char* args);
static void _com_breakpoint(char* args);
static void _com_delete(char* args);
static void _com_info(char* args);
static void _com_disassemble(char* args);
static void _com_step(char* args);
static void _com_continue(char* args);
static void _com_reset(char* args);
static void _com_register(char* args);
static void _com_print(char* args);

typedef struct ugb_command
{
    const char* name;
    void(*func)(char*);
    const char* desc;
} ugb_command;

static ugb_command _commands[] =
{
    { "help",        &_com_help,        "Display this text"  },
    { "?",           &_com_help,        "Synonym for \"help\"" },
    { "quit",        &_com_quit,        "Exit the uGB debugger" },
    { "breakpoint",  &_com_breakpoint,  "Add a new breakpoint" },
    { "delete",      &_com_delete,      "Remove a breakpoint" },
    { "info",        &_com_info,        "List breakpoints" },
    { "disassemble", &_com_disassemble, "Disassemble GB instructions" },
    { "step",        &_com_step,        "Execute a single GB instruction then pause" },
    { "continue",    &_com_continue,    "Continue execution until a breakpoint is hit" },
    { "reset",       &_com_reset,       "Reset the GBM processor" },
    { "register",    &_com_register,    "Examine registers" },
    { "print",       &_com_print,       "Examine memory contents" },
    { 0, 0, 0}
};

char* _skip_whitespace(char* line)
{
    if (!line)
        return 0;

    while (isspace(*line))
        ++line;

    return line;
}

char* _strip_whitespace(char* line)
{
    line = _skip_whitespace(line);

    int len = strlen(line);
    while (isspace(line[len-1]))
        --len;
    line[len] = '\0';

    return line;
}

void _handle_signals(int signo)
{
    if (signo == SIGINT)
    {
        printf("Interrupted.\n");
        _com_disassemble(0);
        siglongjmp(_jmpbuf, 1);
    }
}

void _com_help(char* args)
{
    if (args && *args)
    {
        int found = 0;
        for (ugb_command* cmd = _commands; cmd->name; ++cmd)
        {
            if (!strcmp(cmd->name, args))
            {
                printf("%-10s\t%s.\n", cmd->name, cmd->desc);
                found = 1;
                break;
            }
        }

        if (!found)
        {
            printf("Undefined command: \"%s\".  Try \"help\".\n", args);
        }
    }
    else
    {
        for (ugb_command* cmd = _commands; cmd->name; ++cmd)
        {
            printf("%-10s\t%s.\n", cmd->name, cmd->desc);
        }
    }
}

void _com_quit(char* args)
{
    _quit = 1;
}

void _com_breakpoint(char* args)
{
    if (!args || !*args)
    {
        printf("Expecting breakpoint address.\n");
        return;
    }

    char* end = 0;
    unsigned long in_addr = (unsigned long) strtol(args, &end, 16);

    if (!end || end == args || *end ||
        in_addr > 0xFFFF)
    {
        printf("Invalid address \"%s\".\n", args);
        return;
    }

    uint16_t addr = in_addr;

    for (ugb_breakpoint* bp = _breakpoints; bp; bp = bp->next)
    {
        if (bp->addr == addr)
        {
            printf("Breakpoint #%d also set at 0x%04X.\n", bp->id, addr);
            return;
        }
    }

    int id = ugb_debugger_add_breakpoint(addr);
    printf("Breakpoint #%d set at 0x%04X.\n", id, addr);
}

void _com_delete(char* args)
{
    if (!args || !*args)
    {
        printf("Expecting breakpoint id.\n");
        return;
    }

    char* end = 0;
    int id = strtol(args, &end, 0);

    if (!end || end == args || *end)
    {
        printf("Invalid id \"%s\".\n", args);
        return;
    }

    int err = ugb_debugger_delete_breakpoint(id);

    if (err == UGB_ERR_NOENT)
        printf("No breakpoint #%d.\n", id);
    else
        printf("Removed breakpoint #%d.\n", id);
}

void _com_info(char* args)
{
    for (ugb_breakpoint* bp = _breakpoints; bp; bp = bp->next)
    {
        printf("#%2d at 0x%04X\n", bp->id, bp->addr);
    }
}

void _com_disassemble(char* args)
{
    uint16_t first = *_gbm->cpu->regs.PC;
    uint16_t last = first;

    if (args && *args)
    {
        // Get start address
        char* end = 0;
        unsigned long addr = (unsigned long) strtol(args, &end, 16);

        if (!end || end == args ||
            addr > 0xFFFF)
        {
            printf("Invalid address \"%s\".\n", args);
            return;
        }

        first = addr;
        last = first;

        end = _skip_whitespace(end);
        if (*end == '-')
        {
            char* range = ++end;
            addr = (unsigned long) strtol(range, &end, 16);

            if (!end || end == range ||
                addr > 0xFFFF)
            {
                printf("Invalid address \"%s\".\n", range);
                return;
            }

            last = addr;
        }
        else if (*end == '+')
        {
            char* size = ++end;
            addr = (unsigned long) strtol(size, &end, 0);

            if (!end || end == size ||
                addr > 0xFFFF)
            {
                printf("Invalid size \"%s\".\n", size);
                return;
            }

            last = first + addr;
        }
        else if (*end)
        {
            printf("Invalid address range expression \"%s\".\n", args);
            return;
        }
    }

    if (last < first)
    {
        printf("Invalid address range expression \"%s\".\n", args);
        return;
    }

    char str[256];
    uint8_t data[8];

    uint16_t addr = first;
    while (addr <= last)
    {
        ssize_t len = ugb_read_opcode(&data[0], _gbm, addr);
        if (len <= 0)
        {
            printf("%04X: <%s>\n", addr, ugb_strerror(len));
            ++addr;
        }
        else
        {
            ugb_disassemble(&str[0], sizeof(str), &data[0], addr);

            printf("%04X: %s\n", addr, &str[0]);
            addr += len;
        }
    }
}

void _com_step(char* args)
{
    int err;
    if ((err = ugb_gbm_step(_gbm)) != UGB_ERR_OK)
    {
        printf("Error: %s.\n", ugb_strerror(err));
        return;
    }

    _com_disassemble(0);
}

void _com_continue(char* args)
{
    for (;;)
    {
        int err;
        if ((err = ugb_gbm_step(_gbm)) != UGB_ERR_OK)
        {
            printf("Error: %s.\n", ugb_strerror(err));
            return;
        }

        ugb_breakpoint* match = 0;
        for (ugb_breakpoint* bp = _breakpoints; bp; bp = bp->next)
        {
            if (bp->addr == *_gbm->cpu->regs.PC)
            {
                match = bp;
                break;
            }
        }

        if (match)
        {
            printf("Stopped at breakpoint #%d (0x%04X).\n", match->id, match->addr);
            _com_disassemble(0);
            break;
        }
    }
}

void _com_reset(char* args)
{
    ugb_gbm_reset(_gbm);
    _com_disassemble(0);
}

void _com_register(char* args)
{
    static struct reg_t
    {
        const char* name;
        int word;
        int offset;
    } regs[] =
    {
        #define DEF_REGB(name, offset) { #name, 0, offset },
        #define DEF_REGW(name, offset) { #name, 1, offset },
        #include "cpu.def"

        { 0, 0, 0}
    };

    for (struct reg_t* r = &regs[0]; r->name; ++r)
    {
        if (!strcmp(r->name, args))
        {
            printf("%s = ", r->name);

            void* data_ptr = &_gbm->cpu->regs.data[r->offset];

            if (r->word)
                printf("$%04X", *((uint16_t*) data_ptr));
            else
                printf("$%02X", *((uint8_t*) data_ptr));

            if (r->offset == UGB_REG_F)
            {
                char flags[5] = "____";

                flags[0] = (*_gbm->cpu->regs.F) & UGB_REG_F_Z_MSK ? 'Z' : '_';
                flags[1] = (*_gbm->cpu->regs.F) & UGB_REG_F_N_MSK ? 'N' : '_';
                flags[2] = (*_gbm->cpu->regs.F) & UGB_REG_F_H_MSK ? 'H' : '_';
                flags[3] = (*_gbm->cpu->regs.F) & UGB_REG_F_C_MSK ? 'C' : '_';

                printf(" %s", &flags[0]);
            }

            printf("\n");
            return;
        }
    }

    printf("No register named \"%s\".\n", args);
}

void _com_print(char* args)
{
    if (!args || !*args)
    {
        printf("Expecting address range.\n");
        return;
    }

    uint16_t first = 0;
    uint16_t last = 0;

    // Get start address
    char* end = 0;
    unsigned long addr = (unsigned long) strtol(args, &end, 16);

    if (!end || end == args ||
        addr > 0xFFFF)
    {
        printf("Invalid address \"%s\".\n", args);
        return;
    }

    first = addr;
    last = first;

    end = _skip_whitespace(end);
    if (*end == '-')
    {
        char* range = ++end;
        addr = (unsigned long) strtol(range, &end, 16);

        if (!end || end == range ||
            addr > 0xFFFF)
        {
            printf("Invalid address \"%s\".\n", range);
            return;
        }

        last = addr;
    }
    else if (*end == '+')
    {
        char* size = ++end;
        addr = (unsigned long) strtol(size, &end, 0);

        if (!end || end == size ||
            addr > 0xFFFF)
        {
            printf("Invalid size \"%s\".\n", size);
            return;
        }

        last = first + addr;
    }
    else if (*end)
    {
        printf("Invalid address range expression \"%s\".\n", args);
        return;
    }

    if (last < first)
    {
        printf("Invalid address range expression \"%s\".\n", args);
        return;
    }

    int row = 0;
    int col = 0;

    for (uint16_t addr = first; addr <= last; ++addr)
    {
        if (col == 0)
            printf("%04X: ", addr);

        int err;
        uint8_t value;
        if ((err = ugb_mmu_read(_gbm->mmu, addr, &value)) != UGB_ERR_OK)
        {
            printf("Error: %s\n", ugb_strerror(err));
            return;
        }

        printf("%02X ", value);

        ++col;

        if (!(col % 8))
            printf(" ");

        if (col >= 24 || addr == last)
        {
            printf("\b\n");

            col = 0;
            ++row;
        }
    }
}

int ugb_debugger_add_breakpoint(uint16_t addr)
{
    ugb_breakpoint* bp = malloc(sizeof(ugb_breakpoint));
    if (!bp)
        return UGB_ERR_MALLOC;

    bp->id = _breakpoint_id++;
    bp->addr = addr;

    bp->next = 0;
    bp->prev = _last_breakpoint;
    if (bp->prev)
        bp->prev->next = bp;
    else
        _breakpoints = bp;
    _last_breakpoint = bp;

    return bp->id;
}

int ugb_debugger_delete_breakpoint(int id)
{
    for (ugb_breakpoint* bp = _breakpoints; bp; bp = bp->next)
    {
        if (bp->id == id)
        {
            if (bp->prev)
                bp->prev->next = bp->next;
            else
                _breakpoints = bp->next;

            if (bp->next)
                bp->next->prev = bp->prev;
            else
                _last_breakpoint = bp->prev;

            free(bp);
            return 0;
        }
    }

    return UGB_ERR_NOENT;
}

int ugb_debugger_mainloop(ugb_gbm* gbm)
{
    if (!gbm)
        return UGB_ERR_BADARGS;

    _gbm = gbm;

    while (sigsetjmp(_jmpbuf, 1) != 0);

    for (_quit = 0; !_quit; )
    {
        signal(SIGINT, _handle_signals);
        rl_catch_signals = 1;
        rl_set_signals();

        char* input_line = readline("(ugb) ");

        // Exit on Ctrl+D
        if (!input_line)
        {
            printf("quit\n");
            break;
        }

        // Strip WS
        char* line = _skip_whitespace(input_line);
        char* exec = line;

        // If not empty, record in history
        if (*line)
        {
            add_history(line);
        }
        // Otherwise, get the last recorded command
        else
        {
            HISTORY_STATE* st = history_get_history_state();
            if (st->entries && st->entries[st->length-1])
                exec = st->entries[st->length-1]->line;
        }

        // Execute the actual command
        if (exec && *exec)
        {
            int exec_len = strlen(exec);

            // Split command and arguments
            int cmd_len;
            for (cmd_len = 0; exec[cmd_len] && !isspace(exec[cmd_len]); ++cmd_len);
            exec[cmd_len] = '\0';

            char* args = exec_len > cmd_len ? _strip_whitespace(&exec[cmd_len+1]) : 0;

            // Match the command against our list, handle GDB-like short commands
            ugb_command* match = 0;
            int ok = 1;
            for (ugb_command* cmd = _commands; cmd->name; ++cmd)
            {
                if (!strncmp(cmd->name, exec, cmd_len))
                {
                    int exact = !strcmp(cmd->name, exec);

                    if (match && !exact)
                    {
                        printf("Ambiguous command \"%s\" : ", exec);

                        for (cmd = _commands; cmd->name; ++cmd)
                            if (!strncmp(cmd->name, exec, cmd_len))
                                printf("%s, ", cmd->name);

                        printf("\b\b.\n");
                        ok = 0;
                        break;
                    }
                    else
                    {
                        match = cmd;
                    }
                }
            }

            if (match && ok)
            {
                (*match->func)(args);
            }
            else if (!match)
            {
                printf("Undefined command: \"%s\".  Try \"help\".\n", exec);
            }
        }

        free(input_line);
    }

    _gbm = 0;
    return UGB_ERR_OK;
}

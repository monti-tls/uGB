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

#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <SDL2/SDL.h>

#include "mmu.h"
#include "cpu.h"
#include "hwio.h"
#include "gpu.h"
#include "opcodes.h"
#include "gbm.h"
#include "debugger.h"
#include "constants.h"
#include "errno.h"

void* sdl_main(void* cookie)
{
    ugb_gbm* gbm = (ugb_gbm*) cookie;

    if(SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        printf("SDL_Error: %s\n", SDL_GetError());
        return 0;
    }

    SDL_Window* window = SDL_CreateWindow("uGB",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        3*UGB_GPU_SCREEN_W, 3*UGB_GPU_SCREEN_H,
        SDL_WINDOW_SHOWN);

    if (!window)
    {
        printf("SDL_Error: %s\n", SDL_GetError());
        return 0;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    SDL_RenderSetLogicalSize(renderer, UGB_GPU_SCREEN_W, UGB_GPU_SCREEN_H);

    SDL_Texture* tex = SDL_CreateTexture(renderer,
        SDL_PIXELFORMAT_RGB332,
        SDL_TEXTUREACCESS_STREAMING,
        UGB_GPU_SCREEN_W, UGB_GPU_SCREEN_H);

    for (;;)
    {
        SDL_UpdateTexture(tex, 0, gbm->gpu->framebuf, UGB_GPU_SCREEN_W);

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, tex, 0, 0);
        SDL_RenderPresent(renderer);

        SDL_Delay(100);
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

int sb_hook(ugb_hwreg* reg, void* cookie)
{
    ugb_gbm* gbm = (ugb_gbm*) cookie;

    printf("%c", gbm->hwio->data[UGB_HWIO_REG_SB]);
    return 0;
}

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        printf("Usage: '%s <rom>'.\n", argv[0]);
        return 0;
    }

    // Get input file, map it
    int fd = open(argv[1], O_RDONLY);
    if (fd < 0)
    {
        printf("Unable to open \"%s\".\n", argv[1]);
        return 0;
    }

    struct stat sb;
    fstat(fd, &sb);
    void* file = mmap(0, sb.st_size, PROT_WRITE, MAP_PRIVATE, fd, 0);

    // Create a fresh GameBoy
    ugb_gbm* gbm = ugb_gbm_create();

    // Add the cartridge map
    ugb_mmu_map* rom0 = malloc(sizeof(ugb_mmu_map));
    rom0->low_addr = UGB_CART_ROM0_LO;
    rom0->high_addr = 0x7FFF;
    rom0->type = UGB_MMU_DATA;
    rom0->data = file;
    ugb_mmu_add_map(gbm->mmu, rom0);

    // Map RAM1
    uint8_t ram1_data[0x2000];
    ugb_mmu_map* ram1 = malloc(sizeof(ugb_mmu_map));
    ram1->low_addr = 0xA000;
    ram1->high_addr = 0xBFFF;
    ram1->type = UGB_MMU_DATA;
    ram1->data = &ram1_data[0];
    ugb_mmu_add_map(gbm->mmu, ram1);

    // Map echo internal RAM0
    ugb_mmu_map* ram0 = malloc(sizeof(ugb_mmu_map));
    ram0->low_addr = 0xE000;
    ram0->high_addr = 0xFDFF;
    ram0->type = UGB_MMU_DATA;
    ram0->data = &gbm->mem.ram0[0];
    ugb_mmu_add_map(gbm->mmu, ram0);

    // Map echo internal RAM0
    uint8_t ill_data[0x60];
    ugb_mmu_map* ill = malloc(sizeof(ugb_mmu_map));
    ill->low_addr = 0xFEA0;
    ill->high_addr = 0xFEFF;
    ill->type = UGB_MMU_DATA;
    ill->data = &ill_data[0];
    ugb_mmu_add_map(gbm->mmu, ill);

    ugb_hwio_set_hook(gbm->hwio, UGB_HWIO_REG_SB, &sb_hook, (void*) gbm);

    // Start SDL display thread
    pthread_t disp;
    pthread_create(&disp, 0, &sdl_main, (void*) gbm);

    int err = ugb_debugger_mainloop(gbm);
    if (err < 0)
        printf("error: %s\n", ugb_strerror(err));

    // Cleanup
    ugb_gbm_destroy(gbm);
    munmap(file, sb.st_size);
    close(fd);

    return 0;
}

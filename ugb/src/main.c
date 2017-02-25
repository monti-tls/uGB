#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

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

#include "rom/01_special.h"
#include "rom/02_interrupts.h"
#include "rom/03_op_sp_hl.h"
#include "rom/04_op_r_imm.h"
#include "rom/05_op_rp.h"
#include "rom/06_ld_r_r.h"
#include "rom/07_jr_jp_call_ret_rst.h"
#include "rom/08_misc.h"
#include "rom/09_op_r_r.h"
#include "rom/10_bit_ops.h"
#include "rom/11_op_a_hl.h"
#include "rom/cpu_instrs.h"

uint8_t carthdr_data[] =
{
    0x00, 0xc3, 0x50, 0x01, 0xce, 0xed, 0x66, 0x66, 0xcc, 0x0d, 0x00, 0x0b, 0x03, 0x73, 0x00, 0x83,
    0x00, 0x0c, 0x00, 0x0d, 0x00, 0x08, 0x11, 0x1f, 0x88, 0x89, 0x00, 0x0e, 0xdc, 0xcc, 0x6e, 0xe6,
    0xdd, 0xdd, 0xd9, 0x99, 0xbb, 0xbb, 0x67, 0x63, 0x6e, 0x0e, 0xec, 0xcc, 0xdd, 0xdc, 0x99, 0x9f,
    0xbb, 0xb9, 0x33, 0x3e, 0x54, 0x45, 0x54, 0x52, 0x49, 0x53, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x0b, 0x89, 0xb5,
};

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
    // Create a fresh GameBoy
    ugb_gbm* gbm = ugb_gbm_create();

    // Add the cartridge map
    ugb_mmu_map* rom0 = malloc(sizeof(ugb_mmu_map));
    rom0->low_addr = UGB_CART_ROM0_LO;
    rom0->high_addr = UGB_CART_ROM0_LO + sizeof(ugb_rom_04_op_r_imm) - 1; //0x7FFF;
    rom0->type = UGB_MMU_DATA;
    rom0->data = &ugb_rom_04_op_r_imm[0];
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

    return 0;
}

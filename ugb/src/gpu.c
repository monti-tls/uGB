#include "gpu.h"
#include "hwio.h"
#include "cpu.h"
#include "constants.h"
#include "errno.h"

#include <stdlib.h>
#include <string.h>

ugb_gpu* ugb_gpu_create(ugb_gbm* gbm)
{
    ugb_gpu* gpu = malloc(sizeof(ugb_gpu));
    if (!gpu)
        return 0;

    memset(gpu, 0, sizeof(ugb_gpu));
    gpu->gbm = gbm;

    gpu->mode_clocks[0] = UGB_GPU_MODE00_CLOCKS;
    gpu->mode_clocks[1] = UGB_GPU_MODE01_CLOCKS;
    gpu->mode_clocks[2] = UGB_GPU_MODE10_CLOCKS;
    gpu->mode_clocks[3] = UGB_GPU_MODE11_CLOCKS;

    if (!(gpu->framebuf = malloc(UGB_GPU_SCREEN_W * UGB_GPU_SCREEN_H)) ||
        !(gpu->vram = malloc(UGB_VRAM_SZ)) ||
        !(gpu->oam = malloc(UGB_OAM_SZ)))
    {
        ugb_gpu_destroy(gpu);
        return 0;
    }

    return gpu;
}

void ugb_gpu_destroy(ugb_gpu* gpu)
{
    if (gpu)
    {
        free(gpu->oam);
        free(gpu->vram);
        free(gpu->framebuf);
        free(gpu);
    }
}

int ugb_gpu_reset(ugb_gpu* gpu)
{
    if (!gpu)
        return UGB_ERR_BADARGS;

    //TODO: reset buffers and everything
    gpu->clock = 0;

    return UGB_ERR_OK;
}

static int _render_scanline(ugb_gpu* gpu)
{
    // Get HWIO registers
    int line = gpu->gbm->hwio->data[UGB_HWIO_REG_LY];
    int scy = gpu->gbm->hwio->data[UGB_HWIO_REG_SCY];
    int scx = gpu->gbm->hwio->data[UGB_HWIO_REG_SCX];
    int hwio_lcdc = gpu->gbm->hwio->data[UGB_HWIO_REG_LCDC];
    int hwio_bgp = gpu->gbm->hwio->data[UGB_HWIO_REG_BGP];

    // Get character RAM base and addressing mode
    uint16_t char_ram_base;
    int signed_mode;
    if (hwio_lcdc & (0x01 << 4))
    {
        char_ram_base = 0x0000;
        signed_mode = 0;
    }
    else
    {
        char_ram_base = 0x1000;
        signed_mode = 1;
    }

    // Get BG MAP
    uint16_t bg_map_ram_base;
    if (hwio_lcdc & (0x01 << 3))
        bg_map_ram_base = 0x1C00;
    else
        bg_map_ram_base = 0x1800;

    // Build background palette cache, we store the framebuffer
    //   in the RGB332 format
    // Invert color values (00 -> white, 11 -> black) because the LCD
    //   appears white when OFF
    uint8_t rgb332[4] = { 0xFF, 0x92, 0x49, 0x00 };
    uint8_t palette[4] =
    {
        rgb332[(hwio_bgp >> 0) & 0x3],
        rgb332[(hwio_bgp >> 2) & 0x3],
        rgb332[(hwio_bgp >> 4) & 0x3],
        rgb332[(hwio_bgp >> 6) & 0x3]
    };

    // Go to the current BG MAP line
    // Skip 32 bytes each 8 lines
    bg_map_ram_base += 32 * (((line + scy) & 0xFF) >> 3);

    // Starting tile in the BG MAP
    int bg_map_idx = scx >> 3;

    // Starting positions in the tile
    int y = (line + scy) & 7;
    int x = scx & 7;

    void* tile_idx = &gpu->vram[bg_map_ram_base + bg_map_idx];

    for (int i = 0; i < UGB_GPU_SCREEN_W; ++i)
    {
        // Get the tile data
        uint8_t* tile_data;
        if (signed_mode)
            tile_data = &gpu->vram[char_ram_base + (*((int8_t*) tile_idx) * 16)];
        else
            tile_data = &gpu->vram[char_ram_base + (*((uint8_t*) tile_idx) * 16)];

        // Each 8-pixel tile line is stored using two bytes :
        // 01232100 = 01010100
        //            00111000

        // Rebase on the two bytes for this line
        tile_data += (2 * y);

        // Extract the current pixel
        int b = 7-x;
        uint8_t pixel = (((tile_data[1] & (0x01 << b)) >> b) << 1);
        pixel |= (tile_data[0] & (0x01 << b)) >> b;

        // Run it through the palette and render it to the screen
        gpu->framebuf[line * UGB_GPU_SCREEN_W + i] = palette[pixel];

        // Advance in the tile, get the next one if needed
        if (++x == 8)
        {
            x = 0;
            bg_map_idx = (bg_map_idx + 1) & 0x1F;
            tile_idx = &gpu->vram[bg_map_ram_base + bg_map_idx];
        }
    }

    return UGB_ERR_OK;
}

static int _update_stat_irq(ugb_gpu* gpu)
{
    // Aliases to relevant HWIO registers
    uint8_t hwreg_stat = gpu->gbm->hwio->data[UGB_HWIO_REG_STAT];
    uint8_t hwreg_ly = gpu->gbm->hwio->data[UGB_HWIO_REG_LY];
    uint8_t hwreg_lyc = gpu->gbm->hwio->data[UGB_HWIO_REG_LYC];
    uint8_t* hwreg_if = &gpu->gbm->hwio->data[UGB_HWIO_REG_IF];

    int old_irq = *hwreg_if & UGB_REG_IE_L_MSK;
    int new_irq = 0;

    int mode = hwreg_stat & 0x03;

    if (hwreg_stat & (0x01 << 3))
        new_irq = (mode == 0);
    if (hwreg_stat & (0x01 << 4))
        new_irq = (mode == 1);
    if (hwreg_stat & (0x01 << 5))
        new_irq = (mode == 2);
    if (hwreg_stat & (0x01 << 6))
        new_irq = (hwreg_ly == hwreg_lyc);

    if (!old_irq && new_irq)
    {
        *hwreg_if |= UGB_REG_IE_L_MSK;
    }

    return UGB_ERR_OK;
}

int ugb_gpu_step(ugb_gpu* gpu, size_t cycles)
{
    int err;

    if (!gpu)
        return UGB_ERR_BADARGS;

    // Aliases to relevant HWIO registers
    uint8_t hwreg_lcdc = gpu->gbm->hwio->data[UGB_HWIO_REG_LCDC];
    uint8_t* hwreg_stat = &gpu->gbm->hwio->data[UGB_HWIO_REG_STAT];
    uint8_t* hwreg_ly = &gpu->gbm->hwio->data[UGB_HWIO_REG_LY];
    uint8_t* hwreg_if = &gpu->gbm->hwio->data[UGB_HWIO_REG_IF];

    // Return if not enabled
    //TODO: clear screen first time
    if (!(hwreg_lcdc & (0x01 << 7)))
        return UGB_ERR_OK;

    // Get current mode
    int mode = *hwreg_stat & 0x3;

    // Increment GPU clock, see if we need to do something
    gpu->clock += cycles;
    while (gpu->clock >= gpu->mode_clocks[mode])
    {
        gpu->clock -= gpu->mode_clocks[mode];

        // Emulate GPU hardware
        switch (mode)
        {
            case 2: // OAM read
            {
                // Go to VRAM read mode
                mode = 3;
                break;
            }

            case 3: // VRAM read
            {
                if ((err = _render_scanline(gpu)) != UGB_ERR_OK)
                    return err;

                // Goto HBlank
                mode = 0;
                break;
            }

            case 0: // HBlank
            {
                if (++(*hwreg_ly) >= UGB_GPU_SCREEN_H)
                {
                    // Throw VBlank interrupt
                    *hwreg_if |= UGB_REG_IE_V_MSK;

                    // After the last line, go to the VBlank mode
                    mode = 1;

                    //TODO: send frame to display
                    //TODO: eventually use a double buffer ?
                }
                else
                {
                    // Go to OAM read mode
                    mode = 2;
                }

                break;
            }

            case 1: // VBlank
            {
                if (++(*hwreg_ly) >= UGB_GPU_SCREEN_VH)
                {
                    *hwreg_ly = 0;
                    mode = 2;
                }

                break;
            }
        }

        _update_stat_irq(gpu);
    }

    // Update HWIO registers
    *hwreg_stat = (*hwreg_stat & ~0x3) | mode;

    return UGB_ERR_OK;
}

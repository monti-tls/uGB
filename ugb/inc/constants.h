#ifndef __GBM_CONSTANTS_H__
#define __GBM_CONSTANTS_H__

#include <stdint.h>

#define UGB_GPU_MODE00_CLOCKS 204
#define UGB_GPU_MODE01_CLOCKS 450
#define UGB_GPU_MODE10_CLOCKS 80
#define UGB_GPU_MODE11_CLOCKS 172

#define UGB_GPU_SCREEN_W  160
#define UGB_GPU_SCREEN_H  144
#define UGB_GPU_SCREEN_VH 154

#define UGB_BIOS_LO      0x0000
#define UGB_BIOS_HI      0x00FF
#define UGB_BIOS_SZ      0x0100

#define UGB_CART_ROM0_LO 0x0000
#define UGB_CART_ROM0_HI 0x3FFF
#define UGB_CART_ROM0_SZ 0x4000

#define UGB_RAM0_LO      0xC000
#define UGB_RAM0_HI      0xDFFF
#define UGB_RAM0_SZ      0x2000

#define UGB_HWIO_LO      0xFF00
#define UGB_HWIO_HI      0xFF7F
#define UGB_HWIO_SZ      0x0080

#define UGB_VRAM_LO      0x8000
#define UGB_VRAM_HI      0x9FFF
#define UGB_VRAM_SZ      0x2000

#define UGB_ZPAGE_LO     0xFF80
#define UGB_ZPAGE_HI     0xFFFE
#define UGB_ZPAGE_SZ     0x007F

#define UGB_IEREG_LO     0xFFFF
#define UGB_IEREG_HI     0xFFFF
#define UGB_IEREG_SZ     0x0001

#endif // __GBM_CONSTANTS_H__

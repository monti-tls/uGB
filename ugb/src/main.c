#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "mmu.h"
#include "cpu.h"
#include "opcodes.h"
#include "gbm.h"
#include "debugger.h"
#include "errno.h"

uint8_t bios_data[] =
{
    /* 00000000 */  0x31, 0xfe, 0xff, 0xaf, 0x21, 0xff, 0x9f, 0x32, 0xcb, 0x7c, 0x20, 0xfb, 0x21, 0x26, 0xff, 0x0e,
    /* 00000010 */  0x11, 0x3e, 0x80, 0x32, 0xe2, 0x0c, 0x3e, 0xf3, 0xe2, 0x32, 0x3e, 0x77, 0x77, 0x3e, 0xfc, 0xe0,
    /* 00000020 */  0x47, 0x11, 0x04, 0x01, 0x21, 0x10, 0x80, 0x1a, 0xcd, 0x95, 0x00, 0xcd, 0x96, 0x00, 0x13, 0x7b,
    /* 00000030 */  0xfe, 0x34, 0x20, 0xf3, 0x11, 0xd8, 0x00, 0x06, 0x08, 0x1a, 0x13, 0x22, 0x23, 0x05, 0x20, 0xf9,
    /* 00000040 */  0x3e, 0x19, 0xea, 0x10, 0x99, 0x21, 0x2f, 0x99, 0x0e, 0x0c, 0x3d, 0x28, 0x08, 0x32, 0x0d, 0x20,
    /* 00000050 */  0xf9, 0x2e, 0x0f, 0x18, 0xf3, 0x67, 0x3e, 0x64, 0x57, 0xe0, 0x42, 0x3e, 0x91, 0xe0, 0x40, 0x04,
    /* 00000060 */  0x1e, 0x02, 0x0e, 0x0c, 0xf0, 0x44, 0xfe, 0x90, 0x20, 0xfa, 0x0d, 0x20, 0xf7, 0x1d, 0x20, 0xf2,
    /* 00000070 */  0x0e, 0x13, 0x24, 0x7c, 0x1e, 0x83, 0xfe, 0x62, 0x28, 0x06, 0x1e, 0xc1, 0xfe, 0x64, 0x20, 0x06,
    /* 00000080 */  0x7b, 0xe2, 0x0c, 0x3e, 0x87, 0xe2, 0xf0, 0x42, 0x90, 0xe0, 0x42, 0x15, 0x20, 0xd2, 0x05, 0x20,
    /* 00000090 */  0x4f, 0x16, 0x20, 0x18, 0xcb, 0x4f, 0x06, 0x04, 0xc5, 0xcb, 0x11, 0x17, 0xc1, 0xcb, 0x11, 0x17,
    /* 000000a0 */  0x05, 0x20, 0xf5, 0x22, 0x23, 0x22, 0x23, 0xc9, 0xce, 0xed, 0x66, 0x66, 0xcc, 0x0d, 0x00, 0x0b,
    /* 000000b0 */  0x03, 0x73, 0x00, 0x83, 0x00, 0x0c, 0x00, 0x0d, 0x00, 0x08, 0x11, 0x1f, 0x88, 0x89, 0x00, 0x0e,
    /* 000000c0 */  0xdc, 0xcc, 0x6e, 0xe6, 0xdd, 0xdd, 0xd9, 0x99, 0xbb, 0xbb, 0x67, 0x63, 0x6e, 0x0e, 0xec, 0xcc,
    /* 000000d0 */  0xdd, 0xdc, 0x99, 0x9f, 0xbb, 0xb9, 0x33, 0x3e, 0x3c, 0x42, 0xb9, 0xa5, 0xb9, 0xa5, 0x42, 0x3c,
    /* 000000e0 */  0x21, 0x04, 0x01, 0x11, 0xa8, 0x00, 0x1a, 0x13, 0xbe, 0x20, 0xfe, 0x23, 0x7d, 0xfe, 0x34, 0x20,
    /* 000000f0 */  0xf5, 0x06, 0x19, 0x78, 0x86, 0x23, 0x05, 0x20, 0xfb, 0x86, 0x20, 0xfe, 0x3e, 0x01, 0xe0, 0x50,
    /* 00000100 */
};

uint8_t vram_data[0x2000];
uint8_t zpage_data[0x7F];
uint8_t hwreg_data[0x80];

uint8_t carthdr_data[0x50] =
{
    0x00, // NOP
    0x00, 0x00, // JP PC+? = $0150
    0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B, 0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D,
    0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E, 0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99,
    0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC, 0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E,
};

int main(int argc, char** argv)
{
    ugb_gbm* gbm = ugb_gbm_create();

    /************************/
    /*** Create memory map **/
    /************************/

    ugb_mmu_map* bios = malloc(sizeof(ugb_mmu_map));
    bios->type = UGB_MMU_RODATA;
    bios->rodata = &bios_data[0];
    bios->low_addr = 0x0000;
    bios->high_addr = 0x00FF;
    ugb_mmu_add_map(gbm->mmu, bios);

    ugb_mmu_map* vram = malloc(sizeof(ugb_mmu_map));
    vram->type = UGB_MMU_DATA;
    vram->data = &vram_data[0];
    vram->high_addr = 0x9FFF;
    vram->low_addr = 0x8000;
    ugb_mmu_add_map(gbm->mmu, vram);

    ugb_mmu_map* zpage = malloc(sizeof(ugb_mmu_map));
    zpage->type = UGB_MMU_DATA;
    zpage->data = &zpage_data[0];
    zpage->high_addr = 0xFFFE;
    zpage->low_addr = 0xFF80;
    ugb_mmu_add_map(gbm->mmu, zpage);

    ugb_mmu_map* hwreg = malloc(sizeof(ugb_mmu_map));
    hwreg->type = UGB_MMU_DATA;
    hwreg->data = &hwreg_data[0];
    hwreg->high_addr = 0xFF7F;
    hwreg->low_addr = 0xFF00;
    ugb_mmu_add_map(gbm->mmu, hwreg);

    ugb_mmu_map* carthdr = malloc(sizeof(ugb_mmu_map));
    carthdr->type = UGB_MMU_DATA;
    carthdr->data = &carthdr_data[0];
    carthdr->high_addr = 0x014F;
    carthdr->low_addr = 0x0100;
    ugb_mmu_add_map(gbm->mmu, carthdr);

    int err = ugb_debugger_mainloop(gbm);
    if (err < 0)
        printf("error: %s\n", ugb_strerror(err));

    /***************/
    /*** Cleanup ***/
    /***************/

    ugb_gbm_destroy(gbm);
}

#ifndef __GRAPHICS_CONFIG_H
#define __GRAPHICS_CONFIG_H

#include <Uefi.h>
#include <Library/UefiLib.h>

typedef struct GraphicsConfig {
    uint8_t *framebuffer;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION info;
} GraphicsConfig;

#endif
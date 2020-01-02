#include <Library/UefiLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Uefi.h>
#include <Protocol/DevicePath.h>
#include <Protocol/DevicePathUtilities.h>
#include <Protocol/DevicePathToText.h>
#include <Protocol/DevicePathFromText.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>
#include <Guid/FileInfo.h>

#include <Library/BaseMemoryLib.h>
#include <Library/PrintLib.h>

#include <elf.h>

#include "Loader_lib.c"
#include "LoadElf.c"
#include "GraphicsConfig.h"
#include "MemoryMap.h"

const CHAR16* get_memory_type(EFI_MEMORY_TYPE type) {
    switch (type) {
        case EfiReservedMemoryType:         return L"EfiReservedMemoryType     ";
        case EfiLoaderCode:                 return L"EfiLoaderCode             ";
        case EfiLoaderData:                 return L"EfiLoaderData             ";
        case EfiBootServicesCode:           return L"EfiBootServicesCode       ";
        case EfiBootServicesData:           return L"EfiBootServicesData       ";
        case EfiRuntimeServicesCode:        return L"EfiRuntimeServicesCode    ";
        case EfiRuntimeServicesData:        return L"EfiRuntimeServicesData    ";
        case EfiConventionalMemory:         return L"EfiConventionalMemory     ";
        case EfiUnusableMemory:             return L"EfiUnusableMemory         ";
        case EfiACPIReclaimMemory:          return L"EfiACPIReclaimMemory      ";
        case EfiACPIMemoryNVS:              return L"EfiACPIMemoryNVS          ";
        case EfiMemoryMappedIO:             return L"EfiMemoryMappedIO         ";
        case EfiMemoryMappedIOPortSpace:    return L"EfiMemoryMappedIOPortSpace";
        case EfiPalCode:                    return L"EfiPalCode                ";
        case EfiPersistentMemory:           return L"EfiPersistentMemory       ";
        case EfiMaxMemoryType:              return L"EfiMaxMemoryType          ";
        default:                            return L"InvalidMemoryType         ";
    }
}

// const CHAR16* get_pixel_format(EFI_GRAPHICS_PIXEL_FORMAT fmt) {
//     switch (fmt) {
//         case PixelRedGreenBlueReserved8BitPerColor:
//         return L"PixelRedGreenBlueReserved8BitPerColor";
//         case PixelBlueGreenRedReserved8BitPerColor:
//         return L"PixelBlueGreenRedReserved8BitPerColor";
//         case PixelBitMask:
//         return L"PixelBitMask";
//         case PixelBltOnly:
//         return L"PixelBltOnly";
//         case PixelFormatMax:
//         return L"PixelFormatMax";
//         default:
//         return L"InvalidPixelFormat";
//     }
// }


EFI_STATUS EFIAPI UefiMain(IN EFI_HANDLE ImageHandle,
                           IN EFI_SYSTEM_TABLE *SystemTable) {
    Print(L"Hello EDK II.\n");

    init_protocol();

    EFI_STATUS status;

    gST->ConOut->SetMode(gST->ConOut, gST->ConOut->Mode->MaxMode);

    EFI_LOADED_IMAGE_PROTOCOL *lip = open_protocol(L"LoadedImageProtocol", ImageHandle, ImageHandle, gEfiLoadedImageProtocolGuid);
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs = open_protocol(L"SimpleFileSystem", lip->DeviceHandle, ImageHandle, gEfiSimpleFileSystemProtocolGuid);

    EFI_FILE_PROTOCOL *root_dir;
    fs->OpenVolume(fs, &root_dir);

    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
    UINTN n = 0;
    EFI_HANDLE *gop_handle;
    gBS->LocateHandleBuffer(ByProtocol, &gEfiGraphicsOutputProtocolGuid, NULL, &n, &gop_handle);
    gBS->OpenProtocol(*gop_handle, &gEfiGraphicsOutputProtocolGuid, (void**)&gop, ImageHandle, NULL, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);

    FreePool(gop_handle);

    // Print(L"Resolution: %ux%u, Pixel Format: %s, %u pixels/line\n",
    //     gop->Mode->Info->HorizontalResolution,
    //     gop->Mode->Info->VerticalResolution,
    //     get_pixel_format(gop->Mode->Info->PixelFormat),
    //     gop->Mode->Info->PixelsPerScanLine);
    // Print(L"Frame Buffer: 0x%0lx - 0x%0lx, Size: %lu bytes\n",
    //     gop->Mode->FrameBufferBase,
    //     gop->Mode->FrameBufferBase + gop->Mode->FrameBufferSize,
    //     gop->Mode->FrameBufferSize);

    UINT8* frame_buffer = (UINT8*)gop->Mode->FrameBufferBase;
    for (UINTN i = 0; i < gop->Mode->FrameBufferSize; i ++) {
        frame_buffer[i] = 0xff;
    }


    GraphicsConfig *graphics_config;
    status = gBS->AllocatePool(EfiLoaderData, sizeof(GraphicsConfig), (void**)&graphics_config);
    perror(status, L"allocate pool for graphics_output_protocol_mode", TRUE);
    CopyMem(&graphics_config->info, gop->Mode->Info, sizeof(EFI_GRAPHICS_OUTPUT_MODE_INFORMATION));
    graphics_config->framebuffer = (uint8_t*)gop->Mode->FrameBufferBase;

    VOID *entry;
    load_elf(&entry, root_dir, L"kernel.elf", "_start");


    EFI_MEMORY_DESCRIPTOR memory_descriptor_table[100];
    UINTN buffer_size = sizeof(EFI_MEMORY_DESCRIPTOR) * 100;
    UINTN map_key, descriptor_size; 
    UINT32 descriptor_version;

    status = gBS->GetMemoryMap(&buffer_size, memory_descriptor_table, &map_key, &descriptor_size, &descriptor_version);
    perror(status, L"get memory map", TRUE);

    MemoryMap memory_map = {buffer_size, memory_descriptor_table, map_key, descriptor_size, descriptor_version};
    

    // EFI_MEMORY_DESCRIPTOR *iter;
    // for (iter = memory_map.memory_map; (UINT64)iter < (UINT64)memory_map.memory_map + memory_map.memory_map_size; iter = (EFI_MEMORY_DESCRIPTOR*)((UINTN)iter + memory_map.descriptor_size)) {
    //     Print(L"%lx - %x : %s\n", iter->PhysicalStart, iter->NumberOfPages, get_memory_type(iter->Type));
    // }
    

    // wait_key();

    status = gBS->ExitBootServices(ImageHandle, memory_map.map_key);
    perror(status, L"exit boot service", TRUE);

    typedef void EntryPoint(GraphicsConfig*, MemoryMap*);
    ((EntryPoint*)entry)(graphics_config, &memory_map);


    while(1) {
        __asm__ volatile("hlt");
    }
    return 0;
}


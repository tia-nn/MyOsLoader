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


void wait_key() {
    gBS->WaitForEvent(1, &gST->ConIn->WaitForKey, NULL);
    EFI_INPUT_KEY buf;
    gST->ConIn->ReadKeyStroke(gST->ConIn, &buf);
}


// const CHAR16* get_memory_type(EFI_MEMORY_TYPE type) {
//     switch (type) {
//         case EfiReservedMemoryType:         return L"EfiReservedMemoryType     ";
//         case EfiLoaderCode:                 return L"EfiLoaderCode             ";
//         case EfiLoaderData:                 return L"EfiLoaderData             ";
//         case EfiBootServicesCode:           return L"EfiBootServicesCode       ";
//         case EfiBootServicesData:           return L"EfiBootServicesData       ";
//         case EfiRuntimeServicesCode:        return L"EfiRuntimeServicesCode    ";
//         case EfiRuntimeServicesData:        return L"EfiRuntimeServicesData    ";
//         case EfiConventionalMemory:         return L"EfiConventionalMemory     ";
//         case EfiUnusableMemory:             return L"EfiUnusableMemory         ";
//         case EfiACPIReclaimMemory:          return L"EfiACPIReclaimMemory      ";
//         case EfiACPIMemoryNVS:              return L"EfiACPIMemoryNVS          ";
//         case EfiMemoryMappedIO:             return L"EfiMemoryMappedIO         ";
//         case EfiMemoryMappedIOPortSpace:    return L"EfiMemoryMappedIOPortSpace";
//         case EfiPalCode:                    return L"EfiPalCode                ";
//         case EfiPersistentMemory:           return L"EfiPersistentMemory       ";
//         case EfiMaxMemoryType:              return L"EfiMaxMemoryType          ";
//         default:                            return L"InvalidMemoryType         ";
//     }
// }

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

    CHAR8 map[4096 * 4];
    MemoryMap memmap = {sizeof(map), map, 0, 0, 0, 0};
    
    memmap.map_size = memmap.buffer_size;
    // gBS->GetMemoryMap(&memmap.map_size, (EFI_MEMORY_DESCRIPTOR*)memmap.buffer, &memmap.map_key, &memmap.descriptor_size, &memmap.descriptor_version);

    // Print(L"memmap.buffer = %08lx, memmap.map_size = %08lx\n",
    //   memmap.buffer, memmap.map_size);

    // Print(L"Index, Type, Type(name), PhysicalStart, NumberOfPages, Attribute\n");

    // EFI_PHYSICAL_ADDRESS iter;
    // int i;
    // for (iter = (EFI_PHYSICAL_ADDRESS)memmap.buffer, i = 0; iter < (EFI_PHYSICAL_ADDRESS)memmap.buffer + memmap.map_size; iter += memmap.descriptor_size, i ++) {
    //     EFI_MEMORY_DESCRIPTOR *desc = (EFI_MEMORY_DESCRIPTOR*)iter;
    //     Print(L"%2u, %x, %-ls, %016lx, %016lx, %lx, %lx\n",
    //     i, desc->Type, get_memory_type(desc->Type),
    //     desc->PhysicalStart, desc->PhysicalStart + desc->NumberOfPages * 0x1000 - 1 ,  desc->NumberOfPages,
    //     desc->Attribute & 0xffffflu);
    // }

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

    VOID *entry;
    load_elf(&entry, root_dir, L"kernel.elf");

    status = gBS->GetMemoryMap(&memmap.map_size, (EFI_MEMORY_DESCRIPTOR*)memmap.buffer, &memmap.map_key, &memmap.descriptor_size, &memmap.descriptor_version);
    perror(status, L"get memory map", TRUE);
    status = gBS->ExitBootServices(ImageHandle, memmap.map_key);
    perror(status, L"exit boot service", TRUE);

    typedef void EntryPoint(void *, void*);
    ((EntryPoint*)entry)(frame_buffer, &memmap);


    while(1) {
        __asm__ volatile("hlt");
    }
    return 0;
}


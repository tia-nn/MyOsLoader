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
#include <Protocol/AcpiTable.h>
#include <Guid/Acpi.h>
#include <Guid/FileInfo.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PrintLib.h>

#include <elf.h>

#include "Loader_lib.c"
#include "LoadElf.c"
#include "GraphicsConfig.h"
#include "MemoryMap.h"


EFI_STATUS EFIAPI UefiMain(IN EFI_HANDLE ImageHandle,
                           IN EFI_SYSTEM_TABLE *SystemTable) {
    Print(L"Hello EDK II.\n");
    Print(L"test");

    EFI_STATUS status;
    init_protocol();

    EFI_LOADED_IMAGE_PROTOCOL *lip = open_protocol(L"LoadedImageProtocol", ImageHandle, ImageHandle, gEfiLoadedImageProtocolGuid);
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs = open_protocol(L"SimpleFileSystem", lip->DeviceHandle, ImageHandle, gEfiSimpleFileSystemProtocolGuid);


    // フレームバッファ取得

    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
    UINTN n = 0;
    EFI_HANDLE *gop_handle;
    gBS->LocateHandleBuffer(ByProtocol, &gEfiGraphicsOutputProtocolGuid, NULL, &n, &gop_handle);
    gBS->OpenProtocol(*gop_handle, &gEfiGraphicsOutputProtocolGuid, (void**)&gop, ImageHandle, NULL, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
    FreePool(gop_handle);

    GraphicsConfig *graphics_config;
    status = gBS->AllocatePool(EfiLoaderData, sizeof(GraphicsConfig), (void**)&graphics_config);
    perror(status, L"allocate pool for graphics_output_protocol_mode", TRUE);
    CopyMem(&graphics_config->info, gop->Mode->Info, sizeof(EFI_GRAPHICS_OUTPUT_MODE_INFORMATION));
    graphics_config->framebuffer = (uint8_t*)gop->Mode->FrameBufferBase;


    // ACPIテーブルの取得

    VOID *acpi_table = NULL;    

    for (UINTN i = 0; i < gST->NumberOfTableEntries; i ++) {
        if (guid_equal(&gST->ConfigurationTable[i].VendorGuid, &gEfiAcpiTableGuid)) {
            acpi_table = gST->ConfigurationTable[i].VendorTable;
            break;
        }
    }

    if (acpi_table == NULL) {
        Print(L"ACPI Table not found.\n");
        while (TRUE) __asm__ volatile ("hlt");
    }

    
    // エントリーポイントの作成

    EFI_FILE_PROTOCOL *root_dir;
    fs->OpenVolume(fs, &root_dir);

    VOID *entry, *code_top;
    load_elf(&code_top, &entry, root_dir, L"kernel.elf", "_start");


    EFI_MEMORY_DESCRIPTOR memory_descriptor_table[100];
    UINTN buffer_size = sizeof(EFI_MEMORY_DESCRIPTOR) * 100;
    UINTN map_key, descriptor_size; 
    UINT32 descriptor_version;

    status = gBS->GetMemoryMap(&buffer_size, memory_descriptor_table, &map_key, &descriptor_size, &descriptor_version);
    perror(status, L"get memory map", TRUE);

    MemoryMap memory_map = {buffer_size, memory_descriptor_table, map_key, descriptor_size, descriptor_version};


    // ExitBootServiceはGetMomeryMapの直後

    status = gBS->ExitBootServices(ImageHandle, memory_map.map_key);
    perror(status, L"exit boot service", TRUE);


    typedef void EntryPoint(GraphicsConfig*, MemoryMap*, VOID*, VOID*, VOID*);
    ((EntryPoint*)entry)(graphics_config, &memory_map, acpi_table, code_top, entry);


    while(1) {
        __asm__ volatile("hlt");
    }
    return 0;
}


#ifndef __LOADER_LIB_C
#define __LOADER_LIB_C

#include <Library/DevicePathLib.h>
#include <Library/UefiLib.h>
#include <Protocol/DevicePath.h>
#include <Protocol/DevicePathFromText.h>
#include <Protocol/DevicePathToText.h>
#include <Protocol/DevicePathUtilities.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>
#include <Uefi.h>
#include <Library/UefiBootServicesTableLib.h>

#include <stdint.h>


EFI_DEVICE_PATH_TO_TEXT_PROTOCOL *gDPTTP;
EFI_DEVICE_PATH_FROM_TEXT_PROTOCOL *gDPFTP;
EFI_DEVICE_PATH_UTILITIES_PROTOCOL *gDPUP;
EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *gSFSP;



void wait_key() {
    gBS->WaitForEvent(1, &gST->ConIn->WaitForKey, NULL);
    EFI_INPUT_KEY buf;
    gST->ConIn->ReadKeyStroke(gST->ConIn, &buf);
}


void print_status(IN EFI_STATUS status) {
    switch(status) {
    case EFI_SUCCESS:
        Print(L"Success.");
        break;
    case EFI_INVALID_PARAMETER:
        Print(L"Invalid Parameter.");
        break;
    case EFI_NOT_FOUND:
        Print(L"Not Found");
        break;
    case EFI_UNSUPPORTED:
        Print(L"Unsupported");
        break;
    default:
        Print(L"...default...");
        break;
    }
    return;
}

void perror(EFI_STATUS status, CHAR16 *message, BOOLEAN stop) {
    if(status != EFI_SUCCESS) {
        Print(message);
        Print(L": ");
        print_status(status);
        Print(L"\n");
        if(stop) {
            while(TRUE)
                __asm__ volatile("hlt");
        }
    }
}

void *open_protocol(IN CHAR16 *message,
                    IN EFI_HANDLE handle, IN EFI_HANDLE agentHandle,
                    IN EFI_GUID guid) {
    void *protocol;
    perror(gST->BootServices->OpenProtocol(
               handle, &guid, &protocol, agentHandle, NULL,
               EFI_OPEN_PROTOCOL_GET_PROTOCOL),
           message, TRUE);
    return protocol;
}

void *locate_protocol(IN CHAR16 *message,
                     IN EFI_GUID guid) {
    void *protocol;
    perror(gST->BootServices->LocateProtocol(&guid, NULL, &protocol),
           message, TRUE);
    return protocol;
}

void init_protocol() {

    gDPTTP = (EFI_DEVICE_PATH_TO_TEXT_PROTOCOL *)locate_protocol(
        L"DevicePathToTextProtocol",
        gEfiDevicePathToTextProtocolGuid);

    gDPFTP = (EFI_DEVICE_PATH_FROM_TEXT_PROTOCOL *)locate_protocol(
        L"DevicePathFromTextProtocol",
        gEfiDevicePathFromTextProtocolGuid);

    gDPUP = (EFI_DEVICE_PATH_UTILITIES_PROTOCOL *)locate_protocol(
        L"DevicePathProtocol",
        gEfiDevicePathUtilitiesProtocolGuid);

    gSFSP = (EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *)locate_protocol(
        L"SimpleFileSystemProtocol",
        gEfiSimpleFileSystemProtocolGuid);
}

INT64 strcmp_8(CHAR8 *dest, CHAR8 *src) {
    while (*dest != '\0' && *src != '\0') {
        if (*dest != *src) {
            return 1;
        }
        dest ++;
        src ++;
    }
    if (*dest == *src && *dest == '\0') {
        return 0;
    } else {
        return 1;
    }
}

BOOLEAN guid_equal(EFI_GUID *a, EFI_GUID *b) {
    BOOLEAN result = TRUE;
    result = result && (a->Data1 == b->Data1);
    result = result && (a->Data2 == b->Data2);
    result = result && (a->Data3 == b->Data3);
    for (UINTN i = 0; i < 8; i ++) {
        result = result && (a->Data4[i] == b->Data4[i]);
    }
    return result;
}


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

#endif
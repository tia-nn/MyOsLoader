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

typedef struct MemoryMap {
  unsigned long long buffer_size;
  void* buffer;
  unsigned long long map_size;
  unsigned long long map_key;
  unsigned long long descriptor_size;
  uint32_t descriptor_version;
} MemoryMap;

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

#endif
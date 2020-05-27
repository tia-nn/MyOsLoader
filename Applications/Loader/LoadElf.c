#ifndef __LOAD_ELF_C
#define __LOAD_ELF_C

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



EFI_STATUS load_elf(VOID **buffer_buffer, VOID **entry_point_buffer, EFI_FILE_PROTOCOL *dir, CHAR16 *filename, CHAR8 *entry_point_symbol) {

    EFI_STATUS status;

    EFI_FILE_PROTOCOL *file;
    status = dir->Open(dir, &file, filename, EFI_FILE_MODE_READ, 0);
    perror(status, L"Failed to open elf file.", TRUE);

    VOID *file_buffer;

    UINTN file_size = 0;

    while (TRUE) {
        UINTN size = 100;
        CHAR8 _buf[100];
        file->Read(file, &size, _buf);
        file_size += size;
        if (size < 100) {
            break;
        }
    }
    file->SetPosition(file, 0);

    status = gBS->AllocatePool(EfiLoaderData, file_size, &file_buffer);
    perror(status, L"Failed to allocate pool for file buffer", TRUE);

    status = file->Read(file, &file_size, file_buffer);
    perror(status, L"Failed to read file", TRUE);

    Elf64_Ehdr *elf_header = (Elf64_Ehdr*)file_buffer;
    Elf64_Shdr *section_header_table = (Elf64_Shdr*)((UINT64)file_buffer + elf_header->e_shoff);
    Elf64_Phdr *program_header_table = (Elf64_Phdr*)((UINT64)file_buffer + elf_header->e_phoff);

    CHAR8* section_name_table = (CHAR8*)(file_buffer + section_header_table[elf_header->e_shstrndx].sh_offset);

    UINT64 last_offset;
    last_offset = 0;
    for (UINTN i = 0; i < elf_header->e_phnum; i ++) {
        Elf64_Phdr program_header = program_header_table[i];
        if (program_header.p_type != PT_LOAD) {
            continue;
        }

        last_offset = MAX(last_offset, program_header.p_vaddr + program_header.p_memsz);
    }

    VOID *buffer;
    status = gBS->AllocatePool(EfiLoaderData, last_offset, &buffer);
    perror(status, L"buffer allcate pool", TRUE);


    for (UINTN i = 0; i < elf_header->e_phnum; i ++) {
        Elf64_Phdr program_header = program_header_table[i];
        if (program_header.p_type != PT_LOAD) {
            continue;
        }

        CopyMem(buffer + program_header.p_vaddr, file_buffer + program_header.p_offset, program_header.p_filesz);
        SetMem(buffer + program_header.p_vaddr + program_header.p_filesz,program_header.p_memsz - program_header.p_filesz,  0);
    }


    Elf64_Shdr rela_dyn_section, rela_plt_section, dynsym_section,dynstr_section;// text_section, strtab_section, 

    for (UINTN i = 0; i < elf_header->e_shnum; i ++) {
        Elf64_Shdr section_header = section_header_table[i];

        CHAR8* sec_name = section_name_table + section_header.sh_name;

        // if (strcmp_8(sec_name, ".text") == 0) {
        //     text_section = section_header;
        // } else 
        if (strcmp_8(sec_name, ".rela.dyn") == 0) {
            rela_dyn_section = section_header;
        } else if (strcmp_8(sec_name, ".rela.plt") == 0) {
            rela_plt_section = section_header;
        } else if (strcmp_8(sec_name, ".dynsym") == 0) {
            dynsym_section = section_header;
        // } else if (strcmp_8(sec_name, ".strtab") == 0) {
        //     strtab_section = section_header;
        } else if (strcmp_8(sec_name, ".dynstr") == 0) {
            dynstr_section = section_header;
        }
    }

    Elf64_Sym *sym_table = (Elf64_Sym*)(file_buffer + dynsym_section.sh_offset);
    Elf64_Rela *rela_dyn = (Elf64_Rela*)(file_buffer + rela_dyn_section.sh_offset);
    Elf64_Rela *rela_plt = (Elf64_Rela*)(file_buffer + rela_plt_section.sh_offset);

    for (UINTN i = 0; i < rela_dyn_section.sh_size / sizeof(Elf64_Rela); i ++) {
        Elf64_Sym s = sym_table[ELF64_R_SYM(rela_dyn[i].r_info)];
        Elf64_Rela r = rela_dyn[i];

        void *to = buffer + r.r_offset;
        *(uint64_t*)to = (uint64_t)(buffer + s.st_value);
    }

    for (UINTN i = 0; i < rela_plt_section.sh_size / sizeof(Elf64_Rela); i ++) {
        Elf64_Sym s = sym_table[ELF64_R_SYM(rela_plt[i].r_info)];
        Elf64_Rela r = rela_plt[i];

        void *to = buffer + r.r_offset;
        *(uint64_t*)to = (uint64_t)(buffer + s.st_value);
    }

    UINT64 entry_offset = 0;
    for (UINTN i = 0; i < dynsym_section.sh_size / sizeof(Elf64_Sym); i ++) {
        Elf64_Sym s = sym_table[i];

        if (ELF64_ST_TYPE(s.st_info) == STT_FUNC) {
            CHAR8 *sym_name = ((CHAR8*)(file_buffer + dynstr_section.sh_offset)) + s.st_name;
            if (strcmp_8(sym_name, entry_point_symbol) == 0) {
                entry_offset = s.st_value;
            }
        }
    }

    gBS->FreePool(file_buffer);

    *entry_point_buffer = buffer + entry_offset;
    *buffer_buffer = buffer;

    return EFI_SUCCESS;
}

#endif
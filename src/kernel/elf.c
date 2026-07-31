#include <kernel/elf.h>
#include <kernel/heap.h>
#include <kernel/paging.h>
#include <kernel/serial.h>
#include <kernel/kstring.h>

#define PAGE_SIZE 4096

int elf_load(void *data, uint32_t *entry)
{
        Elf32_Ehdr *ehdr;
        Elf32_Phdr *phdr;
        int i;
        uint32_t offset;

        if (!data || !entry)
                return -1;

        ehdr = (Elf32_Ehdr *)data;

        if (ehdr->e_ident[EI_MAG0] != ELFMAG0 ||
            ehdr->e_ident[EI_MAG1] != ELFMAG1 ||
            ehdr->e_ident[EI_MAG2] != ELFMAG2 ||
            ehdr->e_ident[EI_MAG3] != ELFMAG3) {
                serial_write("[ELF] Bad magic\n");
                return -1;
        }

        if (ehdr->e_ident[EI_CLASS] != ELFCLASS32) {
                serial_write("[ELF] Not 32-bit\n");
                return -1;
        }

        if (ehdr->e_ident[EI_DATA] != ELFDATA2LSB) {
                serial_write("[ELF] Not little-endian\n");
                return -1;
        }

        if (ehdr->e_phnum == 0) {
                serial_write("[ELF] No program headers\n");
                return -1;
        }

        *entry = ehdr->e_entry;
        phdr = (Elf32_Phdr *)((uint8_t *)data + ehdr->e_phoff);

        for (i = 0; i < ehdr->e_phnum; i++) {
                uint32_t paddr;

                if (phdr[i].p_type != PT_LOAD)
                        continue;

                for (offset = 0; offset < phdr[i].p_memsz; offset += PAGE_SIZE) {
                        paddr = alloc_page_frame();
                        if (!paddr) {
                                serial_write("[ELF] Out of memory\n");
                                return -1;
                        }
                        map_page((void *)(phdr[i].p_vaddr + offset),
                                 (void *)paddr,
                                 PTE_PRESENT | PTE_RW | PTE_USER);
                }

                memcpy((void *)phdr[i].p_vaddr,
                       (uint8_t *)data + phdr[i].p_offset,
                       phdr[i].p_filesz);

                if (phdr[i].p_memsz > phdr[i].p_filesz) {
                        memset((void *)(phdr[i].p_vaddr + phdr[i].p_filesz),
                               0,
                               phdr[i].p_memsz - phdr[i].p_filesz);
                }
        }

        return 0;
}

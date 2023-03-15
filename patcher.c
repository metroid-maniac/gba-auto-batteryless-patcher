#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "payload.h"

FILE *romfile;
uint32_t romsize;
uint8_t rom[0x02000000];
char signature[] = "<3 from Maniac";

static uint8_t *memfind(uint8_t *haystack, size_t haystack_size, uint8_t *needle, size_t needle_size, int stride)
{
    for (size_t i = 0; i < haystack_size - needle_size; i += stride)
    {
        if (!memcmp(haystack + i, needle, needle_size)) 
        {
            return haystack + i;
        }
    }
    return NULL;
}

int main(int argc, char **argv)
{
    if (argc != 2) 
    {
        puts("Wrong number of args");
        return 1; 
    }
    
    // Open ROM file
    if (!(romfile = fopen(argv[1], "rb+")))
    {
        puts("Could not open file"):
        return 1;
    }
    
    // Load ROM into memory
    fseek(romfile, 0, SEEK_END);
    romsize = ftell(romfile);
    
    if (romsize > sizeof rom) 
    {
        puts("ROM too large - not a GBA ROM?");
        return 1;
    }
    
    if (romsize & 0xffff)
    {
        puts("ROM not sufficiently aligned - don't trim it");  
    }
    
    fseek(romfile, 0, SEEK_SET);
    fread(rom, 1, romsize, romfile);
    
    // Check if ROM already patched.
    if (memfind(rom, romsize, signature, sizeof signature, 4))
    {
        puts("ROM already patched!");
        return 1;
    }
    
    // Patch all references to IRQ handler address variable
    uint8_t old_irq_addr[4] = { 0xfc, 0x7f, 0x00, 0x03 };
    uint8_t new_irq_addr[4] = { 0xf4, 0x7f, 0x00, 0x03 };
    
    for (uint8_t *p = rom; p < rom + romsize; p += 4)
    {
        if (!memcmp(p, old_irq_addr, sizeof old_irq_addr))
        {
            memcpy(p, new_irq_addr, sizeof new_irq_addr);   
        }
    }
    
    // Find a location to insert the payload immediately before a 0x10000 byte sector
    for (int payload_base = romsize - 0x10000 - sizeof payload; payload_base >= 0; payload_base -= 0x10000)
    {
        int is_all_zeroes = 1;
        int is_all_ones = 1;
        for (int i = 0; i < 0x10000 + sizeof payload; ++i)
        {
            if (rom[payload_base+i] != 0)
            {
                is_all_zeroes = 0;   
            }
            if (rom[payload_base+i] != 0xFF)
            {
                is_all_ones = 0;   
            }
        }
        if (is_all_zeroes || is_all_ones)
        {
            printf("Installing payload at offset %d", payload_base);
            memcpy(rom, payload, sizeof payload);
            
            // Patch the ROM entrypoint to init sram and the dummy IRQ handler, and tell the new entrypoint where the old one was.
            if (rom[3] != 0xea)
            {
                puts("Unexpected entrypoint instruction");
                return 1;
            }
            unsigned long original_entrypoint_offset = rom[0] + rom[1] << 8 + rom[2] << 16;
            unsigned long original_entrypoint_address = 0x08000000 + 8 + (original_entrypoint_offset << 2);
            // little endian assumed, deal with it
            ((uint32_t*) rom)[payload_base + 1[(uint32_t*) payload]] = original_entrypoint_address;
            
            unsigned long new_entrypoint_address = 0x08000000 + payload_base + 0[(uint32_t*) payload];
            0[(uint32_t*) rom] = 0xea000000 | (new_entrypoint_address - 0x08000008) >> 2;

            // Patch any write functions to install the countdown IRQ handler when needed 
            
            // Flush all changes to file
            
            return 0;
        }
    }
    puts("Could not find a location to inject the payload");
    return 1;
}

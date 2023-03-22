#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "payload.h"

FILE *romfile;
uint32_t romsize;
uint8_t rom[0x02000000];
char signature[] = "<3 from Maniac";

// ldr r3, [pc, # 0]; bx r3
static unsigned char thumb_branch_thunk[] = { 0x00, 0x4b, 0x18, 0x47 };
static unsigned char write_sram_signature[] = { 0x30, 0xB5, 0x05, 0x1C, 0x0C, 0x1C, 0x13, 0x1C, 0x0B, 0x4A, 0x10, 0x88, 0x0B, 0x49, 0x08, 0x40};
static unsigned char write_eeprom_signature[] = { 0x70, 0xB5, 0x00, 0x04, 0x0A, 0x1C, 0x40, 0x0B, 0xE0, 0x21, 0x09, 0x05, 0x41, 0x18, 0x07, 0x31, 0x00, 0x23, 0x10, 0x78};
static unsigned char write_flash_signature[] = { 0x70, 0xB5, 0x00, 0x03, 0x0A, 0x1C, 0xE0, 0x21, 0x09, 0x05, 0x41, 0x18, 0x01, 0x23, 0x1B, 0x03};
static unsigned char write_flash2_signature[] = { 0x7C, 0xB5, 0x90, 0xB0, 0x00, 0x03, 0x0A, 0x1C, 0xE0, 0x21, 0x09, 0x05, 0x09, 0x18, 0x01, 0x23};

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
	
	memset(rom, 0x00ff, sizeof rom);

    // Open ROM file
    if (!(romfile = fopen(argv[1], "rb+")))
    {
        puts("Could not open file");
        puts(strerror(errno));
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

    if (romsize & 0x1ffff)
    {
		puts("ROM has been trimmed and is misaligned. Padding to 128KB alignment");
		romsize &= ~0x1ffff;
		romsize += 0x20000;
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
			printf("Found a reference to the IRQ handler address at %lx, patching\n", p - rom);
            memcpy(p, new_irq_addr, sizeof new_irq_addr);
        }
    }

    // Find a location to insert the payload immediately before a 0x20000 byte sector
	int payload_base;
    for (payload_base = romsize - 0x20000 - payload_bin_len; payload_base >= 0; payload_base -= 0x20000)
    {
        int is_all_zeroes = 1;
        int is_all_ones = 1;
        for (int i = 0; i < 0x20000 + payload_bin_len; ++i)
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
           break;
		}
    }
	if (payload_base < 0)
	{
		puts("ROM too small to install payload.");
		if (romsize + 0x40000 > 0x2000000)
		{
			puts("ROM alraedy max size. Cannot expand. Cannot install payload");
			return 1;
		}
		else
		{
			puts("Expanding ROM");
			romsize += 0x40000;
			payload_base = romsize - 0x20000;
		}
	}
	
	printf("Installing payload at offset %x, save file stored at %x\n", payload_base, payload_base + payload_bin_len);
	memcpy(rom + payload_base, payload_bin, payload_bin_len);

	// Patch the ROM entrypoint to init sram and the dummy IRQ handler, and tell the new entrypoint where the old one was.
	if (rom[3] != 0xea)
	{
		puts("Unexpected entrypoint instruction");
		return 1;
	}
	unsigned long original_entrypoint_offset = rom[0];
	original_entrypoint_offset |= rom[1] << 8;
	original_entrypoint_offset |= rom[2] << 16;
	unsigned long original_entrypoint_address = 0x08000000 + 8 + (original_entrypoint_offset << 2);
	printf("Original offset was %lx, original entrypoint was %lx\n", original_entrypoint_offset, original_entrypoint_address);
	// little endian assumed, deal with it
	((uint32_t*) rom)[(payload_base + 1[(uint32_t*) payload_bin]) >> 2] = original_entrypoint_address;

	unsigned long new_entrypoint_address = 0x08000000 + payload_base + 0[(uint32_t*) payload_bin];
	0[(uint32_t*) rom] = 0xea000000 | (new_entrypoint_address - 0x08000008) >> 2;


	// Patch any write functions to install the countdown IRQ handler when needed
	{
		uint8_t *write_location;
		if (write_location = memfind(rom, romsize, write_sram_signature, sizeof write_sram_signature, 2))
		{
			printf("WriteSram identified at offset %lx, patching\n", write_location - rom);
			memcpy(write_location, thumb_branch_thunk, sizeof thumb_branch_thunk);
			1[(uint32_t*) write_location] = 0x08000000 + payload_base + 2[(uint32_t*) payload_bin];
		}
		else if (write_location = memfind(rom, romsize, write_eeprom_signature, sizeof write_eeprom_signature, 2))
		{
			printf("SRAM-patched ProgramEepromDword identified at offset %lx, patching\n", write_location - rom);
			memcpy(write_location, thumb_branch_thunk, sizeof thumb_branch_thunk);
			1[(uint32_t*) write_location] = 0x08000000 + payload_base + 3[(uint32_t*) payload_bin];
		}
		else if (write_location = memfind(rom, romsize, write_flash_signature, sizeof write_flash_signature, 2))
		{
			printf("SRAM-patched flash write function identified at offset %lx\n", write_location - rom);
			memcpy(write_location, thumb_branch_thunk, sizeof thumb_branch_thunk);
			1[(uint32_t*) write_location] = 0x08000000 + payload_base + 4[(uint32_t*) payload_bin];
		}
		else if (write_location = memfind(rom, romsize, write_flash2_signature, sizeof write_flash2_signature, 2))
		{
			printf("SRAM-patched flash write function identified at offset %lx\n", write_location - rom);
			memcpy(write_location, thumb_branch_thunk, sizeof thumb_branch_thunk);
			1[(uint32_t*) write_location] = 0x08000000 + payload_base + 4[(uint32_t*) payload_bin];
		}
		else 
		{
			puts("Could not find a write function to hook. Are you sure the game has save functionality and has been SRAM patched with GBATA?");
			return 1;
		}
	}


	// Flush all changes to file
	fseek(romfile, 0, SEEK_SET);
	fwrite(rom, 1, romsize, romfile);

	return 0;
	
}

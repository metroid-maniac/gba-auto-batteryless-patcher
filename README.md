# Automatic batteryless saving patcher for GBA

This program patches a GBA game for batteryless saving (i.e. saving on a bootleg cartridge containing SRAM but no battery).

The game must be SRAM patched before using this program. GBATA or [Flash1M_Repro_SRAM_patcher](https://github.com/bbsan2k/Flash1M_Repro_SRAM_Patcher) can be used depending on the game. The patch contains two modes, which can be selected during patching.

In auto mode, the save will automatically be flushed a few seconds after the in-game save.

In keypad trigger mode, the save can be flushed by pressing L+R+Start+Select at any time. This mode requires less patching, so may be compatible with more games.

The game will freeze when flushing has started and will unfreeze when flushing has completed.

## Usage
Run with ROM as the only argument, a new ROM will be output. For GUI users, drag the ROM onto the .exe in the file browser.

## Building
No build script, run the following command and hope it doesn't spit out any errors

`$DEVKITARM/bin/arm-none-eabi-gcc -mcpu=arm7tdmi -nostartfiles -nodefaultlibs -mthumb -fPIE -Os -fno-toplevel-reorder payload.c -T payload.ld -o payload.elf; $DEVKITARM/bin/arm-none-eabi-objcopy -O binary payload.elf payload.bin; xxd -i payload.bin > payload_bin.c ; gcc -g patcher.c payload_bin.c`
## Credits
Written by [metroid-maniac](https://github.com/metroid-maniac/)

Thanks to
- [ez-flash](https://github.com/ez-flash) for [EZ Flash Omega kernel](https://github.com/ez-flash/omega-kernel) containing examples for hooking the IRQ handler
- [Fexean](https://gitlab.com/Fexean) for [GBABF](https://gitlab.com/Fexean/gbabf)
- [vrodin](https://github.com/vrodin) for [Burn2Slot](https://github.com/vrodin/Burn2Slot)
- [Lesserkuma](https://github.com/lesserkuma) for [FlashGBX](https://github.com/lesserkuma/FlashGBX) and batteryless versions of [Goomba Color](https://github.com/lesserkuma/goombacolor) and [PocketNES](https://github.com/lesserkuma/PocketNES)
- [Ausar](https://github.com/ArcheyChen) for helping to port the payload to C.

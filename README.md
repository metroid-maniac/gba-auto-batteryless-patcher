# Automatic batteryless saving patcher for GBA

This program patches a GBA game for batteryless saving (i.e. saving on a bootleg cartridge containing SRAM but no battery).

The game must be SRAM patched before using this program. GBATA or [Flash1M_Repro_SRAM_patcher](https://github.com/bbsan2k/Flash1M_Repro_SRAM_Patcher) can be used depending on the game. The patch contains two modes, which can be selected during patching.

In auto mode, the save will automatically be flushed a few seconds after saving. Until the save completes, the graphics will be purposefully corrupted.

In keypad trigger mode, the save can only be flushed by pressing L+R+Start+Select after a save has taken place.

## Usage
Run with ROM as the only argument, the ROM will be modified 

## Building
No build script, run the following command and hope it doesn't spit out any errors

`$DEVKITARM/bin/arm-none-eabi-as -mcpu=arm7tdmi payload.s -o payload.elf; $DEVKITARM/bin/arm-none-eabi-objcopy -O binary payload.elf payload.bin; xxd -i payload.bin > payload.c ; gcc -g *.c`

## Credits
Written by [metroid-maniac](https://github.com/metroid-maniac/)

Thanks to
- [ez-flash](https://github.com/ez-flash) for [EZ Flash Omega kernel](https://github.com/ez-flash/omega-kernel) containing examples for hooking the IRQ handler
- [Fexean](https://gitlab.com/Fexean) for [GBABF](https://gitlab.com/Fexean/gbabf)
- [vrodin](https://github.com/vrodin) for [Burn2Slot](https://github.com/vrodin/Burn2Slot)
- [Lesserkuma](https://github.com/lesserkuma) for [FlashGBX](https://github.com/lesserkuma/FlashGBX) and batteryless versions of [Goomba Color](https://github.com/lesserkuma/goombacolor) and [PocketNES](https://github.com/lesserkuma/PocketNES)

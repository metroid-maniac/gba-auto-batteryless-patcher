# Automatic batteryless saving patcher for GBA

This program patches a GBA game for batteryless saving (i.e. saving on a bootleg cartridge containing SRAM but no battery).

The game must be SRAM patched before using this program. The patch will trigger a countdown lasting a few seconds whenever the game writes to SRAM. When the countdown expires, the patch tries to copy the save data to ROM. While the countdown is taking place, the graphics are purposefully corrupted.

Some games write to SRAM more frequently than you think. To save time, you may play the game in an emulator and observe when saving takes place to determine if it will be too often. Be sure to set the emulator to an SRAM save type if you do this.

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
- [Lesserkuma](https://github.com/lesserkuma) for [FlashGBX](https://github.com/lesserkuma/FlashGBX) and batteryless versions of [Goombe Color](https://github.com/lesserkuma/goombacolor) and [PocketNES](https://github.com/lesserkuma/PocketNES)

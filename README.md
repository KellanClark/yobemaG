# yoBemaG
Game Boy spelled backwards because that makes about as much sense as my code. A crappy, simple, WIP Game Boy emulator I'm trying to write. I'm not even trying to make this cross platform right now, so don't even attempt to run this on a non Linux computer.

## Features
* Full CPU implementation
* Passes the Acid2 test
* Partial timer support
* MBC1, MBC2, MBC3 minus RTC, MBC5 minus rumble, and Wisdom Tree
* Not much else yet. If you were expecting anything I write to be even remotely good/useful, I have some bad news.

## Requirements
* Any modern 64 bit linux distribution (may work on 32 bit but not tested)
* `cmake`, `make`, SDL2 development libraries, Gtk3 development libraries (for `nativefiledialog-extended`), and g++ that supports C++20

## Usage Instructions
To compile:
* `mkdir build`
* `cd build`
* `cmake ..`
* `make`
Run the executable named `yobemag`. If the first argument is not valid, it is treated as the ROM path.

Arguments:
* `--rom <file>`
* `--bootrom <file>` Give path to the bootrom. CPU registers will be set to correct values if not given.
* `--system <dmg/gb, cgb/gbc>` Specify which system to use. This may affect specific features and starting register values. Defaults to Game Boy Color if supported by ROM.
* `--mbc <none, MBC1, MBC2, MBC3, MMM01, MBC5, MBC6, MBC7, wisdomtree>` Override the ROM's MBC. This may have upredictable consequences and shouldn't be used in most circumstances.
* `--sync-to-video` The emulator normally runs as fast as it needs to generate audio samples. This option will try forcing the emulator and GUI to about 60FPS.
* `--unlock-framerate` Run emulator as fast as possible **(only works when syncing to video)**. Using this will cause major audio desync.
* `--log-to-console` Log values of each CPU register to console after every instruction. WARNING: May drastically slow down emulator. It is recommended to pipe the output into a file.

Example: `yobemag Tetris.gb --bootrom bootrom.bin`
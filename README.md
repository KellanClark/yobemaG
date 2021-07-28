# yoBemaG
Game Boy spelled backwards because that makes about as much sense as my code. A crappy, simple, WIP Game Boy emulator I'm trying to write. I'm not even trying to make this cross platform right now, so don't even attempt to run this on a non Linux computer.

## Features
* Full CPU implementation
* Passes the Acid2 test
* Partial timer support
* Full MBC1, full MBC2, much of MBC3, and full MBC5 minus rumble
* Not much else yet. If you were expecting anything I write to be even remotely good/useful, I have some bad news.

## Requirements
* Any modern 64 bit linux distribution (may work on 32 bit but not tested)
* `cmake`, `make`, SDL2, and g++ that supports C++20

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
* `--system <dmg/gb, cgb/gbc>` Specify which system to use. This may affect specific features and starting register values. Defaults to cgb if supported by ROM and not specified.
* `--log-to-console` Log values of each CPU register to console after every instruction. WARNING: May drastically slow down emulator. It is recommended to pipe the output into a file.

Example: `yobemag Tetris.gb --bootrom bootrom.bin`
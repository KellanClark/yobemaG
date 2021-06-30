# yobemaG

Gameboy spelled backwards because that makes about as much sense as my code. A crappy, simple, WIP Gameboy emulator I'm trying to write.
I'm not even trying to make this cross platform right now, so don't even attempt to run this on a non Linux computer.

## Features
* Full CPU implementation
* Passes the Acid2 test
* Partial timer support
* Full MBC1, full MBC2, much of MBC3, and full MBC5 minus rumble
* Not much else yet. If you were expecting anything I write to be even remotely good/useful, I have some bad news.

## Requirements
* Any modern 64 bit linux distribution (may work on 32 bit but not tested)
* A semi-recent version of `gcc` and `make`

## Usage Instructions
Compile using `make` and run the file in `build/` The path of the rom can be given as the first argument or immediately following `--rom`.
Optional Arguments:
* `--bootrom <file>` Use a file as the bootrom. CPU registers will be set to correct values if not given.
* `--system <dmg, cgb>` Specify which CPU to use. 
* `--log-to-console` Log values of each CPU register to console after every instruction. WARNING: May drastically slow down emulator. It is recommended to pipe the output into a file.
eg. `yobemag Tetris.gb --bootrom bootrom.bin`
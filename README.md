# yobemaG

Gameboy spelled backwards because that makes about as much sense as my code. A crappy WIP Gameboy emulator. 
A simple Gameboy emulator I'm trying to write.
I'm not even trying to make this cross platform right now, so don't even attempt to run this on a non Linux computer.

## Requirements
* Any modern 64 bit linux distribution (may work on 32 bit but not tested)
* A semi-recent version of `gcc` and `make`

## Usage Instructions
Compile using `make` and run the file in `build/` with the path of the rom as the first argument.
Optional Arguments:
* `--bootrom <file>` Use a file as the bootrom.
* `--system <dmg, cgb>` Specify which CPU to use.
eg. `yobemag Tetris.gb --bootrom bootrom.bin --no-cycle-accuracy`

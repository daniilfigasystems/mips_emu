# mips_emu
 
I didn't found mips emulator that can almost fully emulate interrupts and portable. So i decided to make own MIPS emulator.
https://github.com/daniilfigasystems/mips_emu
Emulator features:
##################################
Interrupts (note: pc is set to 0x10000180 (due to malloc 4g limit) eret is inaccurate (i can't find what does eret  changes and do so i made my own)
Coprocessor 0
Almost all instructions
MIPS I/II (some instructions from MIPS II)
Basic UART 8550
Portable header only code
##################################

Any contributions for improving emulator allowed!
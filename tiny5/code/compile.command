#!/bin/bash

pushd $(dirname $0) &> /dev/null
directory=$(pwd -P)
popd &> /dev/null

# this is simple enough to not warrant a makefile

# end execution on error
set -e

# begin compiliation
avr-gcc -I. -Os -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums -ffunction-sections -fdata-sections -finline-functions -ansi -pedantic-errors -Wall -Wextra -Wshadow -Werror -mmcu=attiny5 -c $directory/lvc.c -o $directory/lvc.out

avr-gcc -mmcu=attiny5 $directory/lvc.out -o $directory/lvc.elf

avr-objcopy -O ihex -R .eeprom $directory/lvc.elf $directory/lvc.hex

# clean up
rm $directory/lvc.out
rm $directory/lvc.elf

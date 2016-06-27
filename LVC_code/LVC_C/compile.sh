#!/bin/bash

pushd $(dirname $0) &> /dev/null
directory=$(pwd -P)
popd &> /dev/null

avr-gcc -I. -Os -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums -ffunction-sections -fdata-sections -Wall -DF_CPU=8000000UL -mmcu=attiny84 -c $directory/avr_LVC.c -o $directory/avr_LVC.out
avr-gcc -mmcu=attiny84 $directory/avr_LVC.out -o $directory/avr_LVC
avr-objcopy -O ihex -R .eeprom $directory/avr_LVC $directory/avr_LVC.hex

rm $directory/avr_LVC.out
rm $directory/avr_LVC

echo "command to flash the hex file to attiny84 (remember to power the board!): "
echo -e "\e[34msudo avrdude -c atmelice_isp -p t84 -U flash:w:avr_LVC.hex\e[0m"

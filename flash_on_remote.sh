#!/bin/bash

esptool.py --chip esp32 --port "/dev/ttyUSB0" --baud 460800 \
--before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 40m \
--flash_size detect  \
0x1000 /tmp/bootloader.bin \
0x8000 /tmp/partitions.bin  \
0x34d000 /tmp/ota_data_initial.bin  \
0x10000 /tmp/firmware.bin 
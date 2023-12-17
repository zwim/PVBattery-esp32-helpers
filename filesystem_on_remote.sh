#!/bin/bash

esptool.py --chip esp32 --port "/dev/ttyUSB0" --baud 460800 \
--before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 40m \
--flash_size detect  \
0x355000 /tmp/littlefs.bin

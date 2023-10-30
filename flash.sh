esptool.py --chip esp32 --port "/dev/ttyUSB0" --baud 460800 \
--before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 40m \
--flash_size detect  \
0x1000 .pio/build/esp32/bootloader.bin \
0x8000 .pio/build/esp32/partitions.bin  \
0x34d000 .pio/build/esp32/ota_data_initial.bin  \
0x10000 .pio/build/esp32/firmware.bin 

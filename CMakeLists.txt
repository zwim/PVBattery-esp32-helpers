cmake_minimum_required(VERSION 3.16.0)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(espidf-arduino-wifiscan)
littlefs_create_partition_image(spiffs data)
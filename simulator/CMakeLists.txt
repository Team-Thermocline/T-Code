cmake_minimum_required(VERSION 3.13)

# Enable compile_commands.json for IDE IntelliSense
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# initialize pico-sdk from GIT
# (note this can come from environment, CMake cache etc)
set(PICO_SDK_FETCH_FROM_GIT on)

# pico_sdk_import.cmake is a single file copied from this SDK
# note: this must happen before project()
include(pico_sdk_import.cmake)

project(my_project)

# Enable USB stdio for serial communication
set(PICO_STDIO_USB 1)
set(PICO_STDIO_UART 0)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

# rest of your project
add_executable(tcode_simulator main.c)

# Project headers
target_include_directories(tcode_simulator PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include)

# Link pico_stdlib
target_link_libraries(tcode_simulator pico_stdlib)

# create map/bin/hex/uf2 file
pico_add_extra_outputs(tcode_simulator)

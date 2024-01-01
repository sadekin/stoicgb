cmake_minimum_required(VERSION 3.26)
project(stoicgb)

set(CMAKE_CXX_STANDARD 17)

## Manually set SDL2 paths
#set(SDL2_INCLUDE_DIRS "/opt/homebrew/include/SDL2")
#set(SDL2_LIBRARIES "/opt/homebrew/lib/libSDL2.dylib")

find_package(SDL2 REQUIRED)

# Manually set SDL2_ttf paths
set(SDL2_TTF_INCLUDE_DIRS "/opt/homebrew/include/SDL2")
set(SDL2_TTF_LIBRARIES "/opt/homebrew/lib/libSDL2_ttf.dylib")

include_directories(${SDL2_INCLUDE_DIRS} ${SDL2_TTF_INCLUDE_DIRS})

add_executable(stoicgb src/main.cpp
        src/common.hpp
        src/Cartridge.hpp
        src/Cartridge.cpp
        src/Bus.hpp
        src/Bus.cpp
        src/SM83.hpp
        src/SM83.cpp
        src/Ram.hpp
        src/Ram.cpp
        src/UI.hpp
        src/UI.cpp
        src/GB.hpp
        src/GB.cpp
        src/Timer.hpp
        src/Timer.cpp
        src/IO.hpp
        src/IO.cpp
        src/InterruptHandler.hpp
        src/InterruptHandler.cpp
        src/PPU.hpp
        src/PPU.cpp
        src/DMA.hpp
        src/DMA.cpp
        src/LCD.hpp
        src/LCD.cpp
        src/Joypad.hpp
        src/Joypad.cpp
        src/Palette.hpp
        src/Battery.hpp
        src/Battery.cpp
        src/APU.hpp
        src/APU.cpp
        src/MBC.hpp
        src/MBC.cpp
        src/Serial.hpp
        src/Serial.cpp
        lib/tinyfiledialogs/tinyfiledialogs.hpp
        lib/tinyfiledialogs/tinyfiledialogs.cpp
)

target_link_libraries(stoicgb ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARIES})
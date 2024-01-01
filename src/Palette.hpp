#pragma once

#include "common.hpp"

enum GrayScale : uint32_t {
    white       = 0xFFFFFFFF,
    lightGray   = 0xFFAAAAAA,
    darkGray    = 0xFF555555,
    black       = 0xFF000000,
};

constexpr std::array<uint32_t, 4> grayScalePalette = { white, lightGray, darkGray, black };

enum GreenScale : uint32_t {
    whiteGreen  = 0xFF88C070,
    lightGreen  = 0xFFB8F818,
    darkGreen   = 0xFF589000,
    blackGreen  = 0xFF003000,
};

constexpr std::array<uint32_t, 4> greenScalePalette = { whiteGreen, lightGreen, darkGreen, blackGreen };

enum RedScale : uint32_t {
    whiteRed    = 0xFFFF7070,
    lightRed    = 0xFFFFA0A0,
    darkRed     = 0xFFA00000,
    blackRed    = 0xFF300000,
};


enum PastelGreenScale : uint32_t {
    lightPastelGreen    = 0xFFC8F8B8,
    darkPastelGreen     = 0xFF50A850,
    blackPastelGreen    = 0xFF183018,
    whitePastelGreen    = 0xFFB8D8B8
};

constexpr std::array<uint32_t, 4> pastelGreenScalePalette = { whitePastelGreen, lightPastelGreen, darkPastelGreen, blackPastelGreen };

enum PastelYellowScale : uint32_t {
    lightPastelYellow   = 0xFFF8F8A8,
    darkPastelYellow    = 0xFFA8A800,
    blackPastelYellow   = 0xFF303000,
    whitePastelYellow   = 0xFFF8F8B8
};

constexpr std::array<uint32_t, 4> pastelYellowScalePalette = { whitePastelYellow, lightPastelYellow, darkPastelYellow, blackPastelYellow };




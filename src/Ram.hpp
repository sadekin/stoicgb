#pragma once

#include "common.hpp"

class RAM {
public:
    RAM();
    ~RAM();

public:
    uint8_t readWRAM(uint16_t addr);
    void    writeWRAM(uint16_t addr, uint8_t data);

    uint8_t readHRAM(uint16_t addr);
    void    writeHRAM(uint16_t addr, uint8_t data);

private:
    static constexpr uint16_t WRAM_SIZE = 0x2000, WRAM_MEMORY_OFFSET = 0xC000;
    static constexpr uint16_t HRAM_SIZE = 0x0080, HRAM_MEMORY_OFFSET = 0xFF80;

private:
    // From Memory Map =========================================================
    // WRAM (Work RAM):
    // 0xC000 - 0xCFFF : 4 KB Work RAM (WRAM)
    // 0xD000 - 0xDFFF : 4 KB Work RAM (WRAM) - In CGB mode, switchable bank 1~7
    //
    // HRAM (High RAM):
    // 0xFF80 - 0xFFFE : High RAM (HRAM) (Zero Page)
    // =========================================================================
    std::array<uint8_t, WRAM_SIZE> wram;
    std::array<uint8_t, HRAM_SIZE> hram;
};

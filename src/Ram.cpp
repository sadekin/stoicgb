#include "Ram.hpp"

RAM::RAM()
: wram()
, hram() {
    memset(wram.data(), 0, sizeof(wram));
    memset(hram.data(), 0, sizeof(hram));
}

RAM::~RAM() = default;

uint8_t RAM::readWRAM(uint16_t addr) {
    addr -= WRAM_MEMORY_OFFSET;
    if (addr >= WRAM_SIZE) {
        printf("INVALID WRAM ADDR %08X\n", addr + WRAM_MEMORY_OFFSET);
        exit(-1);
    }
    return wram[addr];
}

void RAM::writeWRAM(uint16_t addr, uint8_t data) {
    addr -= WRAM_MEMORY_OFFSET;
    if (addr >= WRAM_SIZE) {
        printf("INVALID WRAM ADDR %08X\n", addr + WRAM_MEMORY_OFFSET);
        exit(-1);
    }
    wram[addr] = data;
}

uint8_t RAM::readHRAM(uint16_t addr) {
    addr -= HRAM_MEMORY_OFFSET;
    if (addr >= HRAM_SIZE) {
        printf("INVALID WRAM ADDR %08X\n", addr + HRAM_MEMORY_OFFSET);
        exit(-1);
    }
    return hram[addr];
}

void RAM::writeHRAM(uint16_t addr, uint8_t data) {
    addr -= HRAM_MEMORY_OFFSET;
    if (addr >= HRAM_SIZE) {
        printf("INVALID WRAM ADDR %08X\n", addr + HRAM_MEMORY_OFFSET);
        exit(-1);
    }
    hram[addr] = data;
}
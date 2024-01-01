#pragma once

#include "common.hpp"
#include "Cartridge.hpp"
#include "Ram.hpp"
#include "IO.hpp"
#include "InterruptHandler.hpp"
#include "Timer.hpp"
#include "PPU.hpp"
#include "DMA.hpp"

class Bus {
public:
    Bus(PPU* p, Cartridge* c, IO* i, InterruptHandler* ih, Timer* t, DMA* d);
    ~Bus();

public: // Bus Read and Write
    uint8_t read(uint16_t addr);
    void    write(uint16_t addr, uint8_t data);

private: // Devices on the bus
    PPU* ppu;
    Cartridge* cartridge;
    IO* io;
    InterruptHandler* intHandler;
    Timer* timer;
    RAM ram;
    DMA* dma;
};
#pragma once

#include "common.hpp"
#include <thread>
#include <chrono>
#include "PPU.hpp"

// Forward declaration of Bus class to prevent circular inclusions.
class Bus;

class DMA {
    friend class LCD; // Needs to read the last written value to the DMA register (addrUpperByte)

public:
    DMA(Bus* b, PPU* p);
    ~DMA();

public:
    void tick();
    void startTransfer(uint8_t addr);
    bool isTransferring() const;

public:
    void ConnectBus(Bus* n) { bus = n; }

private:
    bool    isActive      = false; // Flag to indicate if DMA transfer is active
    uint8_t addrLowerByte = 0x00;  // Counter for current byte being transferred
    uint8_t addrUpperByte = 0x00;  // Upper byte of the DMA source address

private: // Devices connected to the DMA
    Bus* bus = nullptr; // Memory bus for reading data
    PPU* ppu = nullptr; // PPU for writing to OAM
};
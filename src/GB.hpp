#pragma once

#include "common.hpp"
#include "Cartridge.hpp"
#include "Bus.hpp"
#include "SM83.hpp"
#include "UI.hpp"
#include "IO.hpp"
#include "InterruptHandler.hpp"
#include "Timer.hpp"
#include "PPU.hpp"
#include "DMA.hpp"
#include "LCD.hpp"
#include "Joypad.hpp"
#include "APU.hpp"
#include "Serial.hpp"

#include <thread>
#include <chrono>

class SM83;

class GB {
    friend class UI;
    
public:
    GB();
    ~GB();

public:
    void cpuRun();
    void emuRun();
    void emulateCycles(int cpuCycles);

public:
    bool die       = false;
    bool running   = false;
    uint64_t ticks = 0;

public:
    SM83* cpu;
    Bus* bus;
    UI* ui;
    IO* io;
    Timer* timer;
    Cartridge* cartridge;
    InterruptHandler* intHandler;
    PPU* ppu;
    DMA* dma;
    LCD* lcd;
    Joypad* joypad;
    APU* apu;
    Serial* serial;
};

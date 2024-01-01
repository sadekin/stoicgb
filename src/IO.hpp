#pragma once

#include "common.hpp"
#include "Timer.hpp"
#include "InterruptHandler.hpp"
#include "DMA.hpp"
#include "LCD.hpp"
#include "Joypad.hpp"
#include "APU.hpp"
#include "Serial.hpp"

class IO {
public:
    IO(InterruptHandler* ih, Timer* t, DMA* d, LCD* l, Joypad* j, APU* a, Serial* s);
    ~IO();

public:
    uint8_t read(uint16_t addr);
    void    write(uint16_t addr, uint8_t data);

private:
    Timer* timer;
    InterruptHandler* intHandler;
    DMA* dma;
    LCD* lcd;
    Joypad* joypad;
    APU* apu;
    Serial* serial;
};
#pragma once

#include "common.hpp"
#include "InterruptHandler.hpp"
#include "Timer.hpp"

class Serial {
public:
    Serial(InterruptHandler* ih, Timer* t);
    ~Serial();

public:
    void    tick();
    void    init();
    uint8_t read(uint16_t addr) const;
    void    write(uint16_t addr, uint8_t data);

private:
    // Serial Registers
    uint8_t sb = 0x00; // Serial transfer data     (0xFF01)
    uint8_t sc = 0x00; // Serial transfer control  (0xFF02)

private:
    // Assistive variables to facilitate stepping the serial transfer
    bool    fallingEdge    = false; // Indicates whether a selected bit has changed from 1 to 0
    bool    prevBit        = false; // Used for detecting falling edges
    bool    currBit        = false; // Used for detecting falling edges
    bool    clockSelect    = false; // 0 = External clock (“slave”), 1 = Internal clock (“master”)
    bool    clockSpeed     = false; // 0 = Normal speed clock, 1 = Double speed clock
    bool    transferEnable = false; // Indicates whether the serial transfer is enabled according to SC bit 7
    uint8_t shiftCount     = 0x00;  // Number of bits shifted in/out of the SB register

private:
    InterruptHandler* intHandler; // For requesting serial interrupts
    Timer*            timer;      // For accessing the system clock
};
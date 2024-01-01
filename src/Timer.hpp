#pragma once

#include "common.hpp"
#include "InterruptHandler.hpp"

class Timer {
    friend class SM83;   // CPU can directly modify the DIV register (when bypassing boot ROM and manually resetting)
    friend class Serial; // Needs access to system clock

public:
    Timer(InterruptHandler* ih);
    ~Timer();

public:
    void    tick();
    uint8_t read(uint16_t addr) const;
    void    write(uint16_t addr, uint8_t data);

private:
    // Timer Registers
    uint16_t sysClock  = 0x0000; // System Clock (Divider Register is the upper 8 bits)      (0xFF04)
    uint8_t  tima      = 0x00;   // Timer Counter                                            (0xFF05)
    uint8_t  tma       = 0x00;   // Timer Modulo/Reload                                      (0xFF06)
    uint8_t  tac       = 0x00;   // Timer Control                                            (0xFF07)

private:
    // Assistive variables to facilitate stepping the timer
    bool    fallingEdge        = false; // Indicates whether a selected bit has changed from 1 to 0
    bool    prevBit            = false; // Used for detecting falling edges
    bool    currBit            = false; // Used for detecting falling edges
    bool    timerEnable        = false; // Indicates whether the timer is enabled according to TAC bit 2
    uint8_t clockSelect        = 0x00;  // Maps to the frequency at which TIMA is incremented (TAC bits 0-1)
    bool    timaReloading      = false; // Indicates whether we will reload TIMA in the next M-Cycle
    uint8_t ticksSinceOverflow = 0x00;  // Number of ticks since TIMA overflowed (reload TMA after 4 ticks)
    bool    timaReloaded       = false; // Indicates whether TIMA has been reloaded with TMA during current M-Cycle
    uint8_t ticksAfterReload   = 0x00;  // Number of ticks since TIMA was reloaded

    // Maps clockSelect to a corresponding bit as specified by TAC bits 0-1. A change from 1 to 0 in the selected bit
    // represents a falling edge, indicating the completion of the number of clock cycles specified by clockSelect,
    // thus indicating that TIMA should be incremented (assuming timer is enabled).
    const uint16_t clockSelectToBit[4] = {
        1 << 9,  // clockSelect = 0b00 -> 4096   Hz (CPU clock / 1024 = 4194304 / 2^10 = 4096 Hz)
        1 << 3,  // clockSelect = 0b01 -> 262144 Hz (CPU clock / 16   = 4194304 / 2^4  = 262144 Hz)
        1 << 5,  // clockSelect = 0b10 -> 65536  Hz (CPU clock / 64   = 4194304 / 2^6  = 65536  Hz)
        1 << 7   // clockSelect = 0b11 -> 16384  Hz (CPU clock / 256  = 4194304 / 2^8  = 16384  Hz)
    };

    void updateDIV(uint16_t value); // Updates the DIV register
    void detectFallingEdge();       // Falling edge detector, may increment TIMA

private:
    InterruptHandler* intHandler;   // For requesting timer interrupts
};
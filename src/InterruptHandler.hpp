#pragma once

#include "common.hpp"
#include <unordered_map>

class InterruptHandler {
    friend class SM83; // CPU can directly modify the IME flag

public:
    InterruptHandler();
    ~InterruptHandler();

public:
    uint8_t read(uint16_t addr) const;          // Read IF and IE registers
    void    write(uint16_t addr, uint8_t data); // Write IF and IE registers

    // Enumerates the different interrupt types and their corresponding bits in the IF and
    // IE registers. Each type is associated with a specific memory address (source address)
    // that the CPU program counter is set to after successfully handling an interrupt:
    enum InterruptType {
        vBlank  = (1 << 0), // Vertical blanking interrupt           (memory address 0x40)
        lcdStat = (1 << 1), // LCD Status interrupt                  (memory address 0x48)
        timer   = (1 << 2), // Timer (TIMA) overflow interrupt       (memory address 0x50)
        serial  = (1 << 3), // Serial transfer completion interrupt  (memory address 0x58)
        joypad  = (1 << 4), // Joypad state change interrupt         (memory address 0x60)
    };

    void irq(InterruptType it);      // Requests an interrupt of a given type
    bool isr();                      // Interrupt Service Routine: Attempts to handle interrupts based on priority
    bool interruptRequested() const; // Checks if any interrupt was requested and enabled

private:
    bool     IME          = false;  // Interrupt Master Enable flag (enables/disables jumps to interrupt vectors)
    bool     scheduledIME = false;  // For delaying the setting of IME for one M-Cycle (see EI instruction)
    uint8_t  IF           = 0xE1;   // Interrupt Flag Register (bitwise interrupt request flag)             (0xFF0F)
    uint8_t  IE           = 0x00;   // Interrupt Enable Register (bitwise interrupt enabling/disabling)     (0xFFFF)
    uint16_t isrAddress   = 0x0000; // Stores the source address of the most recently handled interrupt

private:
    bool isr(InterruptType it); // Interrupt Service Routine: Attempts to handle a specific interrupt

private:
    // An unordered map linking each InterruptType value to its corresponding interrupt source address.
    std::unordered_map<InterruptType, uint16_t> interruptSourceMap;
    // Helper function to retrieve the source address for a given interrupt type.
    uint16_t getIntSourceAddr(InterruptType it) { return interruptSourceMap[it]; }
};
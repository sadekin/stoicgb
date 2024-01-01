#pragma once

#include "common.hpp"
#include "InterruptHandler.hpp"

class UI;

class Joypad {
    friend class UI; // UI needs to access the states of the button and direction keys

public:
    Joypad(InterruptHandler* ih);
    ~Joypad();

public:
    void    write(uint8_t data); // Sets the selection mode (direction or button) based on the given data
    uint8_t read();              // Reads the joypad selection data

private:
    uint8_t joyp = 0xCF; // Joypad selection data (default 0b11001111) (0xFF00)

private:
    // Direction keys
    bool up     = false;
    bool down   = false;
    bool left   = false;
    bool right  = false;
    // Button keys
    bool a      = false;
    bool b      = false;
    bool start  = false;
    bool select = false;

private:
    bool directionPressed = false;
    bool buttonPressed    = false;
    bool isSelectingDirection() const;  // Returns true if the joypad is selecting the direction keys
    bool isSelectingButton() const;     // Returns true if the joypad is selecting the button keys

private:
    InterruptHandler* intHandler; // Pointer to the interrupt handler for joypad interrupts
};
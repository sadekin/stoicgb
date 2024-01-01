#include "Joypad.hpp"

Joypad::Joypad(InterruptHandler* ih)
: intHandler(ih) {}

Joypad::~Joypad() = default;

/**
 * Sets the selection mode (direction and/or button) based on the given data.
 * See: https://gbdev.gg8.se/wiki/articles/Joypad_Input
 *
 * @param data The data to write.
 */
void Joypad::write(uint8_t data) {
    joyp = data;
    directionPressed = (data & (1 << 4)) == 0; // Bit 4
    buttonPressed    = (data & (1 << 5)) == 0; // Bit 5
}

/**
 * Reads the joypad selection data.
 *
 * Bit 7 - Not used
 * Bit 6 - Not used
 * Bit 5 - P15 Select Button Keys      (0=Select)
 * Bit 4 - P14 Select Direction Keys   (0=Select)
 * Bit 3 - P13 Input Down  or Start    (0=Pressed) (Read Only)
 * Bit 2 - P12 Input Up    or Select   (0=Pressed) (Read Only)
 * Bit 1 - P11 Input Left  or Button B (0=Pressed) (Read Only)
 * Bit 0 - P10 Input Right or Button A (0=Pressed) (Read Only)
 *
 * See: https://gbdev.gg8.se/wiki/articles/Joypad_Input
 *
 * @return The joypad selection data.
 */
uint8_t Joypad::read() {
    // Assume all directions and buttons are not pressed.
    // If a button is pressed, we clear the corresponding bit.
    joyp = 0xCF;

    if (isSelectingButton()) { // Remember, 0=pressed
        if (a)      joyp &= ~(1 << 0);
        if (b)      joyp &= ~(1 << 1);
        if (select) joyp &= ~(1 << 2);
        if (start)  joyp &= ~(1 << 3);

        // If a button is pressed, request an interrupt.
        // See https://gbdev.io/pandocs/Interrupt_Sources.html#int-60--joypad-interrupt
        if (a || b || select || start)
            intHandler->irq(InterruptHandler::joypad);
    }

    if (isSelectingDirection()) { // Remember, 0=pressed
        if (right)  joyp &= ~(1 << 0);
        if (left)   joyp &= ~(1 << 1);
        if (up)     joyp &= ~(1 << 2);
        if (down)   joyp &= ~(1 << 3);
    }

    return joyp | 0b11000000; // Unused bits are always set (bits 6 and 7)
}

/**
 * Checks if the joypad is selecting a direction.
 *
 * @return True if the joypad is selecting a direction, false otherwise.
 */
bool Joypad::isSelectingDirection() const {
    return directionPressed;
}

/**
 * Checks if the joypad is selecting a button.
 *
 * @return True if the joypad is selecting a button, false otherwise.
 */
bool Joypad::isSelectingButton() const {
    return buttonPressed;
}

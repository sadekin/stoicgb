#include "InterruptHandler.hpp"


InterruptHandler::InterruptHandler() {
    // Maps each interrupt flag to its corresponding source address in memory.
    // These addresses are where the interrupt service routines (ISRs) are located.
    interruptSourceMap[vBlank]  = 0x0040;
    interruptSourceMap[lcdStat] = 0x0048;
    interruptSourceMap[timer]   = 0x0050;
    interruptSourceMap[serial]  = 0x0058;
    interruptSourceMap[joypad]  = 0x0060;
}

InterruptHandler::~InterruptHandler() = default;

/**
 * Reads from the Interrupt Enable (IE) or Interrupt Flag (IF) register.
 * See https://gbdev.io/pandocs/Interrupts.html#ffff--ie-interrupt-enable
 * and https://gbdev.io/pandocs/Interrupts.html#ff0f--if-interrupt-flag
 *
 * @param addr The address to read from.
 * @return The byte read from the address.
 */
uint8_t InterruptHandler::read(uint16_t addr) const {
    if (addr == 0xFF0F)
        return IF | 0b11100000; // Bits 5-7 are unused
    if (addr == 0xFFFF)
        return IE; // IE is the only exception to the unused bits rule
    printf("UNSUPPORTED InterruptHandler::read(%04X)\n", addr);
    return 0;
}

/**
 * Writes to the Interrupt Enable (IE) or Interrupt Flag (IF) register.
 * See https://gbdev.io/pandocs/Interrupts.html#ffff--ie-interrupt-enable
 * and https://gbdev.io/pandocs/Interrupts.html#ff0f--if-interrupt-flag
 *
 * @param addr The address to write to.
 * @param data The byte to write.
 */
void InterruptHandler::write(uint16_t addr, uint8_t data) {
    if (addr == 0xFF0F)
        IF = data;
    else if (addr == 0xFFFF)
        IE = data;
    else
        printf("UNSUPPORTED InterruptHandler::read(%04X, %02X)\n", addr, data);
}

/**
 * Interrupt Service Routine (ISR).
 * Checks if an interrupt 'it' is both set in the Interrupt Flag (IF) and enable in the Interrupt Enable (IE) register.
 * I.e., it first checks it 'it' was requested. If so, it handles the interrupt:
 * - Fetches the source address of the interrupt.
 * - Clears the interrupt flag.
 * - Disables further interrupts (disables IME).
 * Note: The current PC is pushed onto the stack and IME is disabled in the CPU::handleInterrupts.
 *
 * @param it The interrupt type to check.
 * @return True if the interrupt was handled.
 */
bool InterruptHandler::isr(InterruptType it) {
    if (IE & IF & it) {
        isrAddress = getIntSourceAddr(it);
        IF &= ~it;
        return true;
    }
    isrAddress = 0x0000; // No interrupt was handled
    return false;
}

/**
 * Interrupt Service Routine (ISR).
 * Attempts to handle each type of interrupt in a predefined priority order.
 * Lower addresses have higher priority. Vblank has the highest priority.
 * Joypad has the lowest priority.
 * See https://gbdev.io/pandocs/Interrupts.html#interrupt-priorities
 *
 * @return True if any interrupt was handled.
 */
bool InterruptHandler::isr() {
    return interruptRequested() && (isr(vBlank) || isr(lcdStat) || isr(timer) || isr(serial) || isr(joypad));
}

/**
 * Requests an interrupt: Sets an interrupt flag 'it' in the Interrupt
 * Flag (IF) register, signaling that an interrupt has occurred.
 *
 * @param it The interrupt type to request.
 */
void InterruptHandler::irq(InterruptType it) {
    IF |= it;
}

/**
 * Returns true if an interrupt was requested (IF flag bit on) and enable (IE flag bit on).
 *
 * @return True if an interrupt was requested and enable.
 */
bool InterruptHandler::interruptRequested() const {
    return IF & IE & 0x1F; // Only the lower 5 bits are used for interrupts
}


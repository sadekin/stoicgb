#include "Serial.hpp"

Serial::Serial(InterruptHandler* ih, Timer* t)
: intHandler(ih)
, timer(t) {}

Serial::~Serial() = default;

void Serial::init() {
    sb = 0x00;
    sc = 0x7E;
}

/**
 * Steps the serial transfer.
 *
 * ------------------------------------------------------------
 * Bits       7          6 5 4 3 2       1             0
 * ------------------------------------------------------------
 * SC  Transfer Enable             Clock Speed    Clock Select
 * ------------------------------------------------------------
 *
 * See https://gbdev.io/pandocs/Serial_Data_Transfer_(Link_Cable).html#serial-data-transfer-link-cable
 */
void Serial::tick() {
    // In Non-CGB Mode the Game Boy supplies an internal clock of 8192 Hz only
    // (allowing to transfer about 1 KByte per second minus overhead for delays).
    // See https://gbdev.io/pandocs/Serial_Data_Transfer_(Link_Cable).html#internal-clock
    currBit = timer->sysClock & (1 << 8); // CPU clock / 8192 = 4194304 / 2^9 = 8192 Hz
    transferEnable = sc & 0x80;           // SC bit 7
    currBit &= transferEnable;
    clockSelect = sc & 0x01;              // SC bit 0
    currBit &= clockSelect;               // Just emulating the internal clock

    clockSpeed = sc & 0x02;               // TODO: SC bit 1 (CGB only, just here for when I implement CGB support)

    fallingEdge = prevBit && !currBit;
    if (fallingEdge) {
        sb = (sb << 1) | 1; // Shift in a 1
        if (++shiftCount == 8) {
            shiftCount = 0;
            sc &= ~0x80; // Clear SC bit 7 (transfer is complete)
            intHandler->irq(InterruptHandler::serial);
        }
    }
    prevBit = currBit;
}

/**
 * Reads from the Serial's registers.
 *
 * @param addr The address to read from.
 * @return The value at the specified address.
 */
uint8_t Serial::read(uint16_t addr) const {
    if (addr == 0xFF01)
        return sb;
    if (addr == 0xFF02)
        return sc | 0b01111110; // Unused bits are always set (bit 1 is CGB only)
    printf("UNMAPPED Serial::read(%04X) \n", addr);
    return 0xFF;
}

/**
 * Writes to the Serial's registers.
 *
 * @param addr The address to write to.
 * @param data The data to write.
 */
void Serial::write(uint16_t addr, uint8_t data) {
    if (addr == 0xFF01)
        sb = data;
    else if (addr == 0xFF02)
        sc = data;
    else
        printf("UNMAPPED Serial::write(%04X)\n", addr);
}
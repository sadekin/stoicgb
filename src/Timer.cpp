#include "Timer.hpp"

Timer::Timer(InterruptHandler* ih)
: intHandler(ih) {}

Timer::~Timer() = default;

/**
 * Advances the state of the Game Boy's timer by one T-Cycle.
 *
 * This method emulates the behavior of the Game Boy's DIV (Divider Register) and TIMA (Timer Counter) registers,
 * incrementing them according to the rules of the original hardware's timing system. The DIV register increments
 * at a fixed rate, while TIMA's increment rate is controlled by the TAC (Timer Control) register.
 *
 * The DIV register increments at 16384 Hz, meaning every 256 T-Cycles, given the CPU clock speed of 4194304 Hz.
 * The upper 8 bits of the 16-bit DIV register are mapped to memory (0xFF04). TIMA increments at a frequency
 * determined by TAC's lower two bits and is enable or disabled by TAC's bit 2:
 *
 * Bit   7 6 5 4 3       2         1 0
 * -----------------------------------------
 * TAC                Enable   Clock Select
 * -----------------------------------------
 * See https://gbdev.io/pandocs/Timer_and_Divider_Registers.html#ff07--tac-timer-control
 *
 * The function handles the incrementing logic for DIV and TIMA, as well as TIMA overflow, triggering a timer interrupt
 * when TIMA overflows and is reloaded with the value in TMA (Timer Modulo).
 */
void Timer::tick() {
    // Check if we've completed the cycle during which TIMA was reloaded with TMA
    if (timaReloaded && ++ticksAfterReload == 4)
        timaReloaded = false; // M-Cycle after TIMA reload is complete

    // Check of we've completed the cycle during which TIMA overflowed.
    // TIMA reads 0x00 for one M-Cycle before being reloaded with TMA
    // and before requesting a timer interrupt.
    if (timaReloading && ++ticksSinceOverflow == 4) {
        tima = tma;                 // Reload TIMA with TMA value
        timaReloading = false;      // M-Cycle after TIMA overflow is complete
        intHandler->irq(InterruptHandler::timer);
        timaReloaded = true;        // TIMA is reloaded with TMA during this M-Cycle
        ticksAfterReload = 0x00;    // Reset
    }

    // Increment DIV every 256 T-Cycles (4194304 Hz / 16384 Hz = 256)
    updateDIV(sysClock + 1);
}

/**
 * Reads from the Timer's registers.
 *
 * @param addr The address to read from.
 * @return The value at the specified address.
 */
uint8_t Timer::read(uint16_t addr) const {
    switch (addr) {
        case 0xFF04: return sysClock >> 8; // Return upper 8-bits of the 16-bit system clock counter
        case 0xFF05: return tima;
        case 0xFF06: return tma;
        case 0xFF07: return (tac & 0x7) | 0xF8;
        default:     return 0xFF;
    }
}

/**
 * Writes to the Timer's registers and accounts for some obscure hardware behavior.
 * See the Cycle-Accurate Game Boy Docs by Antonio Niño Díaz (AntonioND) for more details.
 *
 * @param addr The address to write to.
 * @param data The data to write.
 */
void Timer::write(uint16_t addr, uint8_t data) {
    switch (addr) {
        case 0xFF04:
            // "When writing to DIV register the TIMA register can be increased if the counter has reached half
            // the clocks it needs to increase because the selected bit by the multiplexer will go from 1 to 0
            // (which is a falling edge, that will be detected by the falling edge detector)." - AntonioND
            updateDIV(0x0000); // Having a separate function deals with this edge case of writing to DIV
            break;
        case 0xFF05:
            // "If you write to TIMA during the cycle that TMA is being loaded to it [B],
            // the write will be ignored and TMA value will be written to TIMA instead."
            if (timaReloaded) // TIMA is reloaded with TMA during this M-Cycle
                return;
            tima = data;
            // "During the strange cycle [A] you can prevent the IF flag from being set and prevent the TIMA
            // from being reloaded from TMA by writing a value to TIMA. That new value will be the one that
            // stays in the TIMA register after the instruction. Writing to DIV, TAC or other registers won't
            // prevent the IF flag from being set or TIMA from being reloaded."
            timaReloading = false;
            ticksSinceOverflow = 0x00;
            break;
        case 0xFF06:
            // "If TMA is written the same cycle it is loaded to TIMA [B],
            // TIMA is also loaded with that value."
            tma = data;
            if (timaReloaded) // TIMA is reloaded with TMA during this M-Cycle
                tima = data;
            break;
        case 0xFF07: {
            // "When disabling the timer, if the corresponding bit in the system counter is set to 1,
            // the falling  edge detector will see a change from 1 to 0, so TIMA will increase.
            // This means that whenever half the clocks of the count are reached, TIMA will increase
            // when disabling the timer."
            currBit = prevBit & (data & (1 << 2));
            detectFallingEdge();
            // "When changing TAC register value, if the old selected bit by the multiplexer was 0, the new one is
            // 1, and the new enable bit of TAC is set to 1, it will increase TIMA."
            if (!(prevBit) && (sysClock & clockSelectToBit[data & 0x3]) && (data & (1 << 2))) {
                if (++tima == 0x00) {
                    timaReloading = true;
                    ticksSinceOverflow = 0x00;
                }
            }
            tac = data & 0x7;
        }
            break;
        default:
            break;
    }
}

/**
 * Increments or resets the DIV register.
 * The DIV register is incremented at a rate of 16384 Hz, which means that
 * it's incremented every 256 T-Cycles (4194304 Hz / 16384 Hz = 256).
 * This behavior is facilitated by making DIV the upper 8 bits of the
 * 16-bit `sysClock` register and thus making it possible to increment DIV
 * every 256 T-Cycles.
 * Note that only the upper 8 bits of `sysClock` are mapped to memory (0xFF04)
 * (see Timer::read).
 *
 * @param value The new value of the DIV register (either upper 8 bits of `sysClock + 1`
 *              when the system clock counter is incremented or `0` when DIV is written to).
 */
void Timer::updateDIV(uint16_t value) {
    sysClock = value;

    clockSelect = tac & 0x3;
    currBit = sysClock & clockSelectToBit[clockSelect];
    timerEnable = tac & (1 << 2);
    currBit &= timerEnable;

    // Check if TIMA should be incremented.
    detectFallingEdge();

    prevBit = currBit;
}

/**
 * Falling edge detector. Increments TIMA primarily based on going from a high to
 * a low state in system clock counter (the upper 8 bits of the 16-bit `sysClock` register)
 * based on the frequency specified by the lower two bits of TAC (Timer Control):
 *
 * One condition for incrementing TIMA is when the chosen bit of the `sysClock` register
 * changes from 1 to 0, indicating the completion of the required number of T-Cycles for
 * the selected frequency. For example, if Clock Select is 01, TIMA is incremented every
 * 16 T-Cycles. Note that writing to DIV clears it, so TIMA may be incremented off-cycle.
 * Bits 0 & 1 of Timer Control (TAC) control the frequency at which TIMA is incremented:
 * ------------------------------------------------------------
 *   Clock Select         Base Clock            Frequency
 * ------------------------------------------------------------
 *        00           CPU Clock / 1024         4096   Hz
 *        01           CPU Clock / 16           262144 Hz
 *        10           CPU Clock / 64           65536  Hz
 *        11           CPU Clock / 256          16384  Hz
 * ------------------------------------------------------------
 * See https://gbdev.io/pandocs/Timer_and_Divider_Registers.html#ff07--tac-timer-control
 *
 * The other condition for incrementing TIMA is when the timer is enabled (TAC bit 2).
 *
 * This detector is also used to account for some obscure hardware behavior when disabling
 * the timer by writing to TAC (Timer Control) (see Timer::write).
 */
void Timer::detectFallingEdge() {
    fallingEdge = prevBit && !currBit;
    if (fallingEdge) {
        if (++tima == 0x00) { // Overflow check
            // TIMA reads 0x00 for one M-Cycle before being reloaded with TMA
            // and before requesting a timer interrupt.
            timaReloading = true;      // Set flag indicating that TIMA will be reloaded with TMA in the next M-Cycle
            ticksSinceOverflow = 0x00; // TIMA will be reloaded with TMA after 4 T-Cycles (1 M-Cycle)
        }
    }
}
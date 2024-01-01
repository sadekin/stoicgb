#include "IO.hpp"
#include "Bus.hpp"
#include "Timer.hpp"

IO::IO(InterruptHandler* ih, Timer* t, DMA* d, LCD* l, Joypad* j, APU* a, Serial* s)
: intHandler(ih)
, timer(t)
, dma(d)
, lcd(l)
, joypad(j)
, apu(a)
, serial(s) {}

IO::~IO() = default;

// See: https://gbdev.io/pandocs/Hardware_Reg_List.html
uint8_t IO::read(uint16_t addr) {
    if (addr == 0xFF00)
        return joypad->read();

    if (addr < 0xFF03)
        return serial->read(addr);

    if (addr == 0xFF03)
        printf("UNMAPPED I/O read(%04X) \n", addr);


    if (0xFF04 <= addr && addr <= 0xFF07)
        return timer->read(addr);

    if (addr == 0xFF0F)
        return intHandler->read(addr);

    if (0xFF10 <= addr && addr <= 0xFF3F)
        return apu->read(addr);

    if (0xFF40 <= addr && addr <= 0xFF4B)
        return lcd->read(addr);

//    printf("UNSUPPORTED I/O read(%04X) \n", addr);
    return 0xFF;
}

void IO::write(uint16_t addr, uint8_t data) {
    if (addr == 0xFF00) {
        joypad->write(data);
        return;
    }

    if (addr < 0xFF03) {
        serial->write(addr, data);
        return;
    }

    if (addr == 0xFF03) {
        printf("UNMAPPED I/O write(%04X)\n", addr);
        return;
    }

    if (0xFF04 <= addr && addr <= 0xFF07) {
        timer->write(addr, data);
        return;
    }

    if (addr == 0xFF0F) {
        intHandler->write(addr, data);
        return;
    }

    if (0xFF10 <= addr && addr <= 0xFF3F) {
        apu->write(addr, data);
        return;
    }

    if (0xFF40 <= addr && addr <= 0xFF4B) {
        lcd->write(addr, data);
        return;
    }

//    printf("UNSUPPORTED I/O write(%04X)\n", addr);
}




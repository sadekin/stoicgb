#include "Bus.hpp"
/**
* MEMORY MAP ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* 0x0000 - 0x3FFF : 16KB ROM Bank 00 - From cartridge, usually a fixed bank
* 0x4000 - 0x7FFF : 16KB ROM Bank 01~NN - From cartridge, switchable bank via mapper (if any)
* 0x8000 - 0x9FFF : 8 KB Video RAM (VRAM) - In CGB mode, switchable bank 0/1
*      0x8000 - 0x97FF : CHR RAM
*      0x9800 - 0x9BFF : BG Map 1
*      0x9C00 - 0x9FFF : BG Map 2
* 0xA000 - 0xBFFF : 8 KB External RAM - From cartridge, switchable bank if any
* 0xC000 - 0xCFFF : 4 KB Work RAM (WRAM)
* 0xD000 - 0xDFFF : 4 KB Work RAM (WRAM) - In CGB mode, switchable bank 1~7
* 0xE000 - 0xFDFF : Mirror of C000~DDFF (ECHO RAM) - Nintendo says use of this area is prohibited.
* 0xFE00 - 0xFE9F : Object Attribute Memory (oam)
* 0xFEA0 - 0xFEFF : Not Usable - Nintendo says use of this area is prohibited.
* 0xFF00 - 0xFF7F : I/O Registers
* 0xFF80 - 0xFFFE : High RAM (HRAM) (Zero Page)
* 0xFFFF - 0xFFFF : Interrupt Enable register (IE)
* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* Source: https://gbdev.io/pandocs/Memory_Map.html#memory-map
*/

Bus::Bus(PPU* p, Cartridge* c, IO* i, InterruptHandler* ih, Timer* t, DMA* d)
    : ppu(p), cartridge(c), io(i), intHandler(ih), timer(t), ram(), dma(d) {}

Bus::~Bus() = default;

uint8_t Bus::read(uint16_t addr) {
    if (addr < 0x8000) // ROM data
        return cartridge->read(addr);

    if (addr < 0xA000) // VRAM (Video RAM)
        return ppu->read(addr);

    if (addr < 0xC000) // Cartridge External RAM
        return cartridge->read(addr);

    if (addr < 0xE000) // WRAM (Working RAM)
        return ram.readWRAM(addr);

    if (addr < 0xFE00) // Reserved Echo RAM (unusable)
        return 0;

    if (addr < 0xFEA0) // OAM (Object Attribute Memory)
        return dma->isTransferring() ? 0xFF : ppu->read(addr);

    if (addr < 0xFF00) // Reserved (unusable)
        return 0;

    if (addr < 0xFF80) // I/O Registers
        return io->read(addr);

    if (addr < 0xFFFF) // HRAM (High RAM)
        return ram.readHRAM(addr);

    if (addr == 0xFFFF) // Interrupt Enable register (IE)
        return intHandler->read(addr);

    printf("UNSUPPORTED bus_read(%04X)\n", addr);
    return 0xFF;
}

void Bus::write(uint16_t addr, uint8_t data) {
    if (addr < 0x8000) { // ROM data
        cartridge->write(addr, data);
        return;
    }

    if (addr < 0xA000) { // VRAM (Video RAM)
        ppu->write(addr, data);
        return;
    }

    if (addr < 0xC000) { // Cartridge External RAM
        cartridge->write(addr, data);
        return;
    }

    if (addr < 0xE000) { // WRAM (Working RAM)
        ram.writeWRAM(addr, data);
        return;
    }

    if (addr < 0xFE00) { // Reserved Echo RAM (unusable)
        return;
    }

    if (addr < 0xFEA0) { // OAM (Object Attribute Memory)
        // See https://gbdev.io/pandocs/OAM_DMA_Transfer.html#oam-dma-bus-conflicts
        if (!dma->isTransferring())
            ppu->write(addr, data);
        return;
    }

    if (addr < 0xFF00) { // Reserved (unusable)
        return;
    }

    if (addr == 0xFF50) { // Boot ROM disable
        cartridge->write(addr, data);
        return;
    }

    if (addr < 0xFF80) { // I/O Registers
        io->write(addr, data);
        return;
    }

    if (addr < 0xFFFF) { // HRAM (High RAM)
        ram.writeHRAM(addr, data);
        return;
    }

    if (addr == 0xFFFF) { // Interrupt Enable register (IE)
        intHandler->write(addr, data);
        return;
    }

    printf("UNSUPPORTED bus_write(%04X)\n", addr);
}



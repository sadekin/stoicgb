#include "MBC.hpp"
#include "Cartridge.hpp"
#include "Battery.hpp"

MBC::MBC(uint8_t* pRom, Cartridge* pCartridge) : rom(pRom), cartridge(pCartridge), ramBanks() {}

MBC::MBC(uint8_t *pRom, int nRomBanks, int nRamBanks, Cartridge* pCartridge)
: rom(pRom)
, romBanksCount(nRomBanks)
, ramBanksCount(nRamBanks)
, cartridge(pCartridge)
, ramBanks() {
    // Code     SRAM size    Comment
    // ----------------------------------------------
    // $00      None         No RAM
    // $01      -            Unused
    // $02      8 KiB        1 bank
    // $03      32 KiB       4 banks of 8 KiB each
    // $04      128 KiB      16 banks of 8 KiB each
    // $05      64 KiB       8 banks of 8 KiB each
    // ----------------------------------------------
    // Max 16 banks of 8 KiB each.
    // See https://gbdev.io/pandocs/The_Cartridge_Header.html#0149--ram-size
    for (int i = 0; i < ramBanksCount; i++) {
        ramBanks[i] = new uint8_t[0x2000]; // 0x2000 = 2 * 16^3 = 2 * 4096 = 2 * 4KB = 8KB
        memset(ramBanks[i], 0x00, 0x2000);
    }
    ramBank = ramBanks[0];   // Default to the first RAM bank
    romBankX = rom + 0x4000; // Default to the first switchable ROM bank (0x4000 = 4 * 16^3 = 4 * 4094 = 4 * 4KB = 16KB)
}

MBC::~MBC() = default;

// MBC0 (No MBC) =======================================================================================================
uint8_t MBC0::read(uint16_t addr) const {
    if (addr < 0x8000)
        return rom[addr];
    return 0xFF;
}

void MBC0::write(uint16_t addr, uint8_t data) {
    // No MBC registers to write to
    // ROM is read-only. Do nothing.
}
// =====================================================================================================================


// MBC1 ================================================================================================================
uint8_t MBC1::read(uint16_t addr) const {
    // First 16KB (Bank 00) of cartridge ROM (unable to be switched or modified)
    if (addr < 0x4000)
        return rom[addr];
    // ROM Bank 01-7F (Read-Only)
    if (addr < 0x8000)
        return romBankX[addr - 0x4000];
    // RAM Bank 00-03, if any (External Cartridge RAM)
    if (ramEnabled && ramBank && 0xA000 <= addr && addr < 0xC000) {
        return ramBank[addr - 0xA000];
    }
    return 0xFF;
}

void MBC1::write(uint16_t addr, uint8_t data) {
    // RAM Enable (Write-Only)
    if (addr < 0x2000) {
        ramEnabled = (data & 0xF) == 0xA;
        return;
    }
    // ROM Bank Number (Write-Only)
    if (addr < 0x4000) {
        data &= 0x1F; // Only the lower 5 bits are used (0-31)
        if (data == 0x00)
            data = 0x01;
        romBankNum = data;
        romBankX = rom + (0x4000 * (romBankNum % romBanksCount));
        return;
    }
    // RAM Bank Number (Write-Only)
    if (addr < 0x6000) {
        ramBankNum = data & 0x3; // Only the lower 2 bits are used
        if (modeSelect) { // RAM Banking Mode (sort of misleading for multi-carts)
            if (cartridge->needsToSave())
                cartridge->save();
            ramBank = ramBanks[ramBankNum % ramBanksCount];
        }
        return;
    }
    // Banking Mode Select (Write-Only)
    if (addr < 0x8000) {
        modeSelect = data & 0x01;
        if (modeSelect) { // RAM Banking Mode
            if (cartridge->needsToSave())
                cartridge->save();
            ramBank = ramBanks[ramBankNum % ramBanksCount];
        }
        return;
    }
    // Switchable RAM Bank 00-03, if any (External Cartridge RAM) (Read/Write)
    if (ramEnabled && ramBank && 0xA000 <= addr && addr < 0xC000) {
        ramBank[addr - 0xA000] = data;
        if (cartridge->hasBattery())
            cartridge->setNeedsSave();
        return;
    }
}

// =====================================================================================================================


// MBC2 ================================================================================================================
MBC2::MBC2(uint8_t* pRom, int nRomBanks, int nRamBanks, Cartridge* pCartridge)
: ram()
, MBC1(pRom, nRomBanks, nRamBanks, pCartridge) {
    hasInternalRam = true;
}

uint8_t MBC2::read(uint16_t addr) const {
    // First 16KB (Bank 00) of cartridge ROM (unable to be switched or modified)
    if (addr < 0x4000)
        return rom[addr];
    // ROM Bank $01-0F (Read Only)
    if (addr < 0x8000)
        return romBankX[addr - 0x4000];
    // Internal RAM
    if (ramEnabled && 0xA000 <= addr && addr < 0xC000) {
        return ram[addr & 0x1FF] | 0xF0; // Upper 2 bits of all read values are always 1
    }
    return 0xFF;
}

void MBC2::write(uint16_t addr, uint8_t data) {
    // RAM Enable, ROM Bank Number (Write-Only)
    if (addr < 0x4000) {
        if ((data & 0x0100) == 0) {
            ramEnabled = (data & 0xF) == 0xA;
        } else {
            romBankNum = ((data & 0x1F) == 0) ? 1 : data & 0x1F;
            romBankX = rom + (0x4000 * (romBankNum % romBanksCount));
        }
        return;
    }
    // Internal RAM
    if (ramEnabled && 0xA000 <= addr && addr < 0xC000) {
        ram[addr & 0x1FF] = data & 0xF;
        if (cartridge->hasBattery())
            cartridge->setNeedsSave();
        return;
    }
}
// =====================================================================================================================


// MBC3 ================================================================================================================
uint8_t MBC3::read(uint16_t addr) const {
    // First 16KB (Bank 00) of cartridge ROM (unable to be switched or modified)
    if (addr < 0x4000)
        return rom[addr];
    // ROM Bank $01-7F (Read-Only)
    if (addr < 0x8000)
        return romBankX[addr - 0x4000];
    // RAM Bank 00-03, if any (External Cartridge RAM)
    if (ramEnabled && ramBank && 0xA000 <= addr && addr < 0xC000) {
        return ramBank[addr - 0xA000];
    }
    return 0xFF;
}

void MBC3::write(uint16_t addr, uint8_t data) {
    // Enable RAM and RTC Registers (Write-Only)
    if (addr < 0x2000) {
        ramEnabled = (data & 0xF) == 0xA;
        return;
    }
    // ROM Bank Number (Write-Only)
    if (addr < 0x4000) {
        data &= 0x7F; // Only the lower 7 bits are used
        if (data == 0x00)
            data = 0x01;
        romBankNum = data;
        romBankX = rom + (0x4000 * (romBankNum % romBanksCount));
        return;
    }
    // RAM Bank Number or TODO: RTC Register Select (Write-Only)
    if (addr < 0x6000) {
        data &= 0x0F; // Only the lower 4 bits are used
        if (ramEnabled && data <= 0x03) {
            ramBankNum = data;
            if (cartridge->needsToSave())
                cartridge->save();
            ramBank = ramBanks[ramBankNum % ramBanksCount];
        }
        return;
    }
    // TODO: Latch Clock Data (Write-Only)
    if (addr < 0x8000) {
        return;
    }
    // RAM Bank 00-03, if any (External Cartridge RAM)
    if (ramEnabled && ramBank && 0xA000 <= addr && addr < 0xC000) {
        ramBank[addr - 0xA000] = data;
        if (cartridge->hasBattery())
            cartridge->setNeedsSave();
        return;
    }
}
// =====================================================================================================================

// MBC5 ================================================================================================================
MBC5::MBC5(uint8_t* pRom, int nRomBanks, int nRamBanks, Cartridge* pCartridge)
: MBC1(pRom, nRomBanks, nRamBanks, pCartridge) {
    // Bank 0 is actually bank 0 here
    romBankNum = 0x00;
    romBankX = rom;
}

uint8_t MBC5::read(uint16_t addr) const {
    // First 16KB (Bank 00) of cartridge ROM (unable to be switched or modified)
    if (addr < 0x4000)
        return rom[addr];
    // ROM Bank $00-1FF (Read-Only)
    if (addr < 0x8000)
        return romBankX[addr - 0x4000];
    // RAM Bank 00-0F, if any (External Cartridge RAM)
    if (ramEnabled && ramBank && 0xA000 <= addr && addr < 0xC000) {
        return ramBank[addr - 0xA000];
    }
    return 0xFF;
}

void MBC5::write(uint16_t addr, uint8_t data) {
    // RAM Enable (Write-Only)
    if (addr < 0x2000) {
        ramEnabled = (data & 0xF) == 0xA;
        return;
    }
    // ROM Bank Number Low (Write-Only)
    if (addr < 0x3000) {
        romBankNum = (ramBankNum & 0xFF00) | data; // Sets lower 8 bits of romBankNum
        romBankX = rom + (0x4000 * (romBankNum % romBanksCount));
        return;
    }
    // ROM Bank Number High (Write-Only)
    if (addr < 0x4000) {
        romBankNum = (romBankNum & 0x00FF) | ((data & 0x1) << 8); // Sets bit 9 of romBankNum
        romBankX = rom + (0x4000 * (romBankNum % romBanksCount));
        return;
    }
    // RAM Bank Number (Write-Only)
    if (addr < 0x6000) {
        ramBankNum = data & 0xF; // Only the lower 4 bits are used (0-15)
        if (cartridge->needsToSave())
            cartridge->save();
        ramBank = ramBanks[ramBankNum % ramBanksCount];
        return;
    }
    // Banking Mode Select (Write-Only)
    if (addr < 0x8000) {
        modeSelect = data & 0x01;
        if (modeSelect) { // RAM Banking Mode
            if (cartridge->needsToSave())
                cartridge->save();
            ramBank = ramBanks[ramBankNum % ramBanksCount];
        }
        return;
    }
    // RAM Bank 00-0F, if any (External Cartridge RAM)
    if (ramEnabled && ramBank && 0xA000 <= addr && addr < 0xC000) {
        ramBank[addr - 0xA000] = data;
        if (cartridge->hasBattery())
            cartridge->setNeedsSave();
        return;
    }
}
#pragma once

#include <cstdint>
#include "common.hpp"

class Cartridge;
class Battery;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class MBC {
    friend class Cartridge;
    friend class Battery;

public:
    MBC(uint8_t* pRom, Cartridge* pCartridge);
    MBC(uint8_t *pRom, int nRomBanks, int nRamBanks, Cartridge* pCartridge);
    ~MBC();

public:
    virtual uint8_t read(uint16_t addr) const = 0;
    virtual void    write(uint16_t addr, uint8_t data) = 0;

protected:
    uint8_t* rom;
    uint8_t* romBankX = nullptr; // Pointer to the current ROM bank
    int romBanksCount = 1;

    std::array<uint8_t*, 16> ramBanks;
    uint8_t* ramBank = nullptr; // Pointer to the current RAM bank
    int ramBanksCount = 0;

    bool hasInternalRam = false; // For MBC2

protected:
    Cartridge* cartridge;
};
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class MBC0 : public MBC { // No MBC
public:
    using MBC::MBC; // Inherit constructors from MBC

public:
    uint8_t read(uint16_t addr) const override;
    void    write(uint16_t addr, uint8_t data) override;
};
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class MBC1 : public MBC {
public:
    using MBC::MBC; // Inherit constructors from MBC

public:
    uint8_t read(uint16_t addr) const override;
    void    write(uint16_t addr, uint8_t data) override;

protected:
    bool ramEnabled = false;    // RAM enable/disable
    bool modeSelect = false;    // Banking mode select (0=simple/default, 1=advanced)
    uint16_t romBankNum = 0x01; // 5-bit register. Selects ROM bank for the 0x4000-0x7FFF address range
    uint16_t ramBankNum = 0x00; // Current RAM bank. Determines which 8KB block of external RAM is mapped to 0xA000-0xBFFF
};
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class MBC2 : public MBC1 {
public:
    MBC2(uint8_t *pRom, int nRomBanks, int nRamBanks, Cartridge *pCartridge);

public:

    uint8_t read(uint16_t addr) const override;
    void    write(uint16_t addr, uint8_t data) override;

public:
    uint8_t ram[512]; // Internal RAM (512 half-bytes)
};
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class MBC3 : public MBC1 {
public:
    using MBC1::MBC1;

public:
    uint8_t read(uint16_t addr) const override;
    void    write(uint16_t addr, uint8_t data) override;
};
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class MBC5 : public MBC1 {
public:
    MBC5(uint8_t* pRom, int nRomBanks, int nRamBanks, Cartridge* pCartridge);

public:
    uint8_t read(uint16_t addr) const override;
    void    write(uint16_t addr, uint8_t data) override;
};
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
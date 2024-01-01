#pragma once

#include "common.hpp"
#include "MBC.hpp"

class Battery;

class Cartridge {
    friend class Battery;
    friend class GB; // For disabling boot ROM

public:
    Cartridge(const std::string& filePath);
    ~Cartridge();

public:
    uint8_t read(uint16_t addr);
    void    write(uint16_t addr, uint8_t data);

public:
    bool needsToSave() const; // Does the cartridge need to be saved?
    void setNeedsSave();      // Set flag to indicate that the cartridge needs to be saved
    bool hasBattery();        // Check if the cartridge has a battery
    void save();              // Save the cartridge to disk

private:
    // See: https://gbdev.io/pandocs/The_Cartridge_Header.html
    struct Header {                          // <Address range>: <Description>
        std::array<char, 4>  entryPoint;     // 0x0100-0x0103  : Initial execution point
        std::array<char, 48> nintendoLogo;   // 0x0104-0x0133  : Nintendo logo bitmap
        std::array<char, 15> title;          // 0x0134-0x0142  : Game title in ASCII
        uint8_t  cgbFlag;                    // 0x0143         : Color GB support flag
        uint16_t newLicenseeCode;            // 0x0144-0x0145  : New publisher code
        uint8_t  sgbFlag;                    // 0x0146         : Super GB support flag
        uint8_t  cartridgeType;              // 0x0147         : Cartridge feature type
        uint8_t  romSize;                    // 0x0148         : Size of ROM
        uint8_t  ramSize;                    // 0x0149         : Size of external RAM
        uint8_t  destinationCode;            // 0x014A         : Japan or international
        uint8_t  oldLicenseeCode;            // 0x014B         : Old publisher code
        uint8_t  version;                    // 0x014C         : Game version number
        uint8_t  headerChecksum;             // 0x014D         : Header checksum
        uint16_t globalChecksum;             // 0x014E-0x014F  : Global ROM checksum
    };

private:
    uint32_t romSize;           // Size of the ROM data
    uint8_t* rom;               // Buffer for the ROM data
    int      romBanksCount;     // Number of ROM banks
    int      ramBanksCount;     // Number of RAM banks
    bool     needsSave = false; // Does the cartridge need to be saved?

private:
    Header*     header;         // Pointer to the header within rom
    std::string fileName;       // Filename of the loaded ROM
    uint8_t*    bootROM;        // Buffer for the boot ROM data
    bool        bootROMEnabled; // Is the boot ROM enabled?

private:
    MBC*     mbc;               // Memory Bank Controller
    Battery* battery = nullptr; // Pointer to the battery (if any) for saving/loading RAM


private:
    bool        load();             // Load ROM into memory
    MBC*        getMBC();           // Get the memory bank controller
    int         getRAMBanksCount(); // Get the number of RAM banks
    std::string getCartType();      // Get the cartridge type
    std::string getCartLicence();   // Get the cartridge licence
    bool        checksumPassed();   // Check if the header checksum is valid
};
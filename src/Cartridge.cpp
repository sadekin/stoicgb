#include "Cartridge.hpp"
#include "Battery.hpp"

// The offset in the Game Boy ROM where the cartridge header starts.
// In Game Boy ROMs, the header starts at 0x0100, which is 256 bytes into the ROM.
static constexpr uint16_t HEADER_START_OFFSET = 0x0100;

// Source: https://gbdev.io/pandocs/The_Cartridge_Header.html#01440145--new-licensee-code
static const std::map<uint8_t, std::string> LIC_CODE = {
        { 0x00, "None" },
        { 0x01, "Nintendo R&D1" },
        { 0x08, "Capcom" },
        { 0x13, "Electronic Arts" },
        { 0x18, "Hudson Soft" },
        { 0x19, "b-ai" },
        { 0x20, "kss" },
        { 0x22, "pow" },
        { 0x24, "PCM Complete" },
        { 0x25, "san-x" },
        { 0x28, "Kemco Japan" },
        { 0x29, "seta" },
        { 0x30, "Viacom" },
        { 0x31, "Nintendo" },
        { 0x32, "Bandai" },
        { 0x33, "Ocean/Acclaim" },
        { 0x34, "Konami" },
        { 0x35, "Hector" },
        { 0x37, "Taito" },
        { 0x38, "Hudson" },
        { 0x39, "Banpresto" },
        { 0x41, "Ubi Soft" },
        { 0x42, "Atlus" },
        { 0x44, "Malibu" },
        { 0x46, "angel" },
        { 0x47, "Bullet-Proof" },
        { 0x49, "irem" },
        { 0x50, "Absolute" },
        { 0x51, "Acclaim" },
        { 0x52, "Activision" },
        { 0x53, "American sammy" },
        { 0x54, "Konami" },
        { 0x55, "Hi tech entertainment" },
        { 0x56, "LJN" },
        { 0x57, "Matchbox" },
        { 0x58, "Mattel" },
        { 0x59, "Milton Bradley" },
        { 0x60, "Titus" },
        { 0x61, "Virgin" },
        { 0x64, "LucasArts" },
        { 0x67, "Ocean" },
        { 0x69, "Electronic Arts" },
        { 0x70, "Infogrames" },
        { 0x71, "Interplay" },
        { 0x72, "Broderbund" },
        { 0x73, "sculptured" },
        { 0x75, "sci" },
        { 0x78, "THQ" },
        { 0x79, "Accolade" },
        { 0x80, "misawa" },
        { 0x83, "lozc" },
        { 0x86, "Tokuma Shoten Intermedia" },
        { 0x87, "Tsukuda Original" },
        { 0x91, "Chunsoft" },
        { 0x92, "Video system" },
        { 0x93, "Ocean/Acclaim" },
        { 0x95, "Varie" },
        { 0x96, "Yonezawa/s’pal" },
        { 0x97, "Kaneko" },
        { 0x99, "Pack in soft" },
        { 0xA4, "Konami (Yu-Gi-Oh!)" }
};

// Source: https://gbdev.io/pandocs/The_Cartridge_Header.html#0147--cartridge-type
static const std::array<std::string, 35> ROM_TYPES = {
        "ROM ONLY",                               // 0x00
        "MBC1",                                   // 0x01
        "MBC1+RAM",                               // 0x02
        "MBC1+RAM+BATTERY",                       // 0x03
        "0x04 ???",                               // 0x04
        "MBC2",                                   // 0x05
        "MBC2+BATTERY",                           // 0x06
        "0x07 ???",                               // 0x07
        "ROM+RAM 1",                              // 0x08
        "ROM+RAM+BATTERY 1",                      // 0x09
        "0x0A ???",                               // 0x0A
        "MMM01",                                  // 0x0B
        "MMM01+RAM",                              // 0x0C
        "MMM01+RAM+BATTERY",                      // 0x0D
        "0x0E ???",                               // 0x0E
        "MBC3+timer+BATTERY",                     // 0x0F
        "MBC3+timer+RAM+BATTERY 2",               // 0x10
        "MBC3",                                   // 0x11
        "MBC3+RAM 2",                             // 0x12
        "MBC3+RAM+BATTERY 2",                     // 0x13
        "0x14 ???",                               // 0x14
        "0x15 ???",                               // 0x15
        "0x16 ???",                               // 0x16
        "0x17 ???",                               // 0x17
        "0x18 ???",                               // 0x18
        "MBC5",                                   // 0x19
        "MBC5+RAM",                               // 0x1A
        "MBC5+RAM+BATTERY",                       // 0x1B
        "MBC5+RUMBLE",                            // 0x1C
        "MBC5+RUMBLE+RAM",                        // 0x1D
        "MBC5+RUMBLE+RAM+BATTERY",                // 0x1E
        "0x1F ???",                               // 0x1F
        "MBC6",                                   // 0x20
        "0x21 ???",                               // 0x21
        "MBC7+SENSOR+RUMBLE+RAM+BATTERY"          // 0x22
};

Cartridge::Cartridge(const std::string &filePath) {
    // Save the filename for loading/saving ROM/RAM.
    fileName = filePath;

    // Load and read game ROM.
    if (!load())
        exit(-1);

    if (bootROMEnabled) {
        // Load and read boot ROM.
        std::ifstream bootromFile("../roms/DMG_ROM.bin", std::ios::binary); // Open file in binary mode
        if (!bootromFile) {
            std::cerr << "Failed to open: dmg_boot.bin\n";
            std::cerr << "Exiting...\n";
            exit(-1);
        }
        bootromFile.seekg(0, std::ios::beg);
        bootROM = new uint8_t[0x100]; // First 256 bytes
        bootromFile.read(reinterpret_cast<char*>(bootROM), 0x100);
        bootromFile.close();
    }

    // Set up memory bank controller and battery (if any).
    mbc = getMBC();
    if (hasBattery()) {
        battery = new Battery(this);
        battery->load();
    }
}

Cartridge::~Cartridge() = default;

/**
 * Reads a byte from the cartridge or the cartridge's memory bank controller (if any).
 *
 * @param addr The address to read from.
 * @return The byte at the given address.
 */
uint8_t Cartridge::read(uint16_t addr) {
    if (bootROMEnabled && addr < 0x100) {
        return bootROM[addr];
    }
    return mbc->read(addr);
}


/**
 * Writes a byte to the cartridge's memory bank controller (if any).
 * ROM is read-only, so writing to ROM will do nothing.
 *
 * @param addr The address to write to.
 * @param data The byte to write.
 */
void Cartridge::write(uint16_t addr, uint8_t data) {
    if (bootROMEnabled && addr < 0x100) {
        return;
    }
    if (addr == 0xFF50) { // See https://gbdev.gg8.se/wiki/articles/Gameboy_Bootstrap_ROM
        bootROMEnabled = false;
        return;
    }
    mbc->write(addr, data);
}

/**
 * Attempts to loads the ROM into memory and logs information about the cartridge.
 *
 * @return True if the ROM was successfully loaded, false otherwise.
 */
bool Cartridge::load() {
    std::ifstream file(fileName, std::ios::binary); // Open file in binary mode
    if (!file) { // Check if file was successfully opened
        std::cerr << "Failed to open: " << fileName << "\n";
        std::cerr << "Exiting...\n";
        return false;
    }
    // Seek to the end of the file to find its size
    file.seekg(0, std::ios::end);
    romSize = static_cast<uint32_t>(file.tellg());

    if (romSize % 0x4000 != 0) { // 0x4000 = 4 * 16^3 = 4 * 4096 = 4 * 4 KB = 16 KB
        std::cerr << "Invalid ROM size: " << romSize <<".\n";
        std::cerr << "Size must be a multiple of 16 KB.\n";
        return false;
    }

    rom = new uint8_t[romSize];                         // Allocate memory for the ROM data
    romBanksCount = static_cast<int>(romSize) / 0x4000; // 16 KB per bank

    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char*>(rom), romSize);   // Read the entire file into the ROM buffer
    file.close();

    // Point the header to the correct location in the ROM data
    header = reinterpret_cast<Header*>(rom + HEADER_START_OFFSET);

    ramBanksCount = getRAMBanksCount();

    // Using printf() because it's easier to format.
    printf("Successfully loaded ROM from file path: %s:\n", fileName.c_str());
    printf("\tTitle    : %s\n", header->title.data());
    printf("\tType     : %2.2X (%s)\n", header->cartridgeType, getCartType().c_str());
    printf("\tROM Size : %d KB\n", 32 << header->romSize);
    printf("\tROM Banks: %d\n", romBanksCount);
    printf("\tRAM Size : %2.2X\n", header->ramSize);
    printf("\tRAM Banks: %d\n", ramBanksCount);
    printf("\tLIC Code : %2.2X (%s)\n", header->oldLicenseeCode, getCartLicence().c_str());
    printf("\tROM Vers : %2.2X\n", header->version);
    printf("\tChecksum : %2.2X (%s)\n", header->headerChecksum, checksumPassed() ? "PASSED" : "FAILED");

    return true;
}

/**
 * Checks if the cartridge has a battery.
 *
 * @return True if the cartridge has a battery, false otherwise.
 */
bool Cartridge::hasBattery() {
    return header->cartridgeType == 0x03 || // MBC1+RAM+BATTERY
           header->cartridgeType == 0x06 || // MBC2+BATTERY
           header->cartridgeType == 0x09 || // ROM+RAM+BATTERY 1
           header->cartridgeType == 0x0D || // MMM01+RAM+BATTERY
           header->cartridgeType == 0x0F || // MBC3+timer+BATTERY
           header->cartridgeType == 0x10 || // MBC3+timer+RAM+BATTERY 2
           header->cartridgeType == 0x13 || // MBC3+RAM+BATTERY 2
           header->cartridgeType == 0x1B || // MBC5+RAM+BATTERY
           header->cartridgeType == 0x1E || // MBC5+RUMBLE+RAM+BATTERY
           header->cartridgeType == 0x22;   // MBC7+SENSOR+RUMBLE+RAM+BATTERY
}

/**
 * Gets the cartridge type.
 *
 * @return The cartridge type or "UNKNOWN" if the type is not known.
 */
std::string Cartridge::getCartType() {
    if (header->cartridgeType <= 0xA4)
        return ROM_TYPES[header->cartridgeType];
    return "UNKNOWN";
}

/**
 * Gets the cartridge licence.
 *
 * @return The cartridge licence or "UNKNOWN" if the licence is not known.
 */
std::string Cartridge::getCartLicence() {
    auto it = LIC_CODE.find(header->oldLicenseeCode);
    if (it != LIC_CODE.end())
        return it->second;
    return "UNKNOWN";
}

/**
 * Checks if the header checksum is valid.
 * See https://gbdev.io/pandocs/The_Cartridge_Header.html#014d--header-checksum
 *
 * @return True if the header checksum is valid, false otherwise.
 */
bool Cartridge::checksumPassed() {
    uint16_t checksum{};
    for (uint16_t address = 0x0134; address <= 0x014C; address++)
        checksum -= rom[address] + 1;
    return header->headerChecksum == (checksum & 0xFF);
}

/**
 * Saves the cartridge data to the battery.
 */
void Cartridge::save() {
    if (battery != nullptr && needsSave)
        battery->save();
    needsSave = false; // To avoid saving too often
}

/**
 * Checks if the cartridge needs to be saved.
 *
 * @return True if the cartridge needs to be saved, false otherwise.
 */
bool Cartridge::needsToSave() const {
    return needsSave && battery != nullptr;
}

/**
 * Sets the needsSave flag to true.
 */
void Cartridge::setNeedsSave() {
    needsSave = true;
}

/**
 * Code     SRAM size    Comment
 * ----------------------------------------------
 * $00      None         No RAM
 * $01      -            Unused
 * $02      8 KiB        1 bank
 * $03      32 KiB       4 banks of 8 KiB each
 * $04      128 KiB      16 banks of 8 KiB each
 * $05      64 KiB       8 banks of 8 KiB each
 * ----------------------------------------------
 * Max 16 banks of 8 KiB each.
 * See https://gbdev.io/pandocs/The_Cartridge_Header.html#0149--ram-size
 *
 * @return The number of RAM banks.
 */
int Cartridge::getRAMBanksCount() {
    switch (header->ramSize) {
        case 0x00:
        case 0x01: return 0;
        case 0x02: return 1;
        case 0x03: return 4;
        case 0x04: return 16;
        case 0x05: return 8;
        default:
            std::cerr << "Invalid RAM size: " << header->ramSize << ".\n";
            std::cerr << "RAM size must be in the range [0x00, 0x05].\n";
            exit(-1);
    }
}

/**
 * Gets the appropriate memory bank controller based on the cartridge's type.
 * TODO: Only MBC0 (no MBC) and MBC1-MBC3 are supported at the moment.
 *
 * @return The memory bank controller.
 */
MBC* Cartridge::getMBC() {
    // MBC(uint8_t *pRom, int nRomBanks, int nRamBanks, Cartridge* pCartridge);
    switch (header->cartridgeType) {
        case 0x00: return new MBC0(rom, this);
        case 0x01:
        case 0x02:
        case 0x03: return new MBC1(rom, romBanksCount, ramBanksCount, this);
        case 0x05:
        case 0x06: return new MBC2(rom, romBanksCount, ramBanksCount, this);
        case 0x0F:
        case 0x10:
        case 0x11:
        case 0x12:
        case 0x13: return new MBC3(rom, romBanksCount, ramBanksCount, this);
        case 0x19:
        case 0x1A:
        case 0x1B: return new MBC5(rom, romBanksCount, ramBanksCount, this);
        default:
            std::cerr << "Unsupported MBC: " << getCartType() << ".\n";
            exit(-1);
    }
}
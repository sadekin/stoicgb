#pragma once

#include "common.hpp"
#include "Cartridge.hpp"
#include "LCD.hpp"
#include "InterruptHandler.hpp"

#include <stdexcept>
#include <queue>

class PPU {
    friend class LCD;
    friend class DMA;
    friend class UI;
    friend class GB;

public:
    PPU(Cartridge* c, LCD* l, InterruptHandler* ih);
    ~PPU();

public:
    void    tick(); // Updates the PPU state and handles the current PPU mode.
    uint8_t read(uint16_t addr);
    void    write(uint16_t addr, uint8_t data);

private:
    uint16_t dots           = 0x0000;     // Counts the amount of time passed (a dot/tick is a PPU time unit)
    uint32_t framesRendered = 0x00000000; // Number of frames processed, used for synchronization and timing
    std::array<uint32_t, 160 * 144> videoBuffer; // Holds pixel data for the current frame, used for rendering.

private:
    // This struct represents a sprite's (OAM entry) data as stored in the Game Boy's Object Attribute Memory (OAM).
    // Each sprite is defined by four bytes: (see https://gbdev.io/pandocs/OAM.html#object-attribute-memory-oam)
    struct Sprite {
        uint8_t yPos;       // Byte 0: Y position of the sprite on the screen + 16
        uint8_t xPos;       // Byte 1: X position of the sprite on the screen + 8
        uint8_t tileNum;    // Byte 2: Tile number of the sprite (0-255) in the tile set
        struct Attributes { // Byte 3: Attributes/flags for the attributes (bit-field representation)
            uint8_t cgbPalette : 3; // - CGB Palette number (0-7)
            uint8_t bank       : 1; // - Tile VRAM Bank number (0 or 1 in CGB Mode)
            uint8_t dmgPalette : 1; // - DMG Palette number (0 or 1, Non-CGB Mode Only)
            uint8_t xFlip      : 1; // - X flip (0 = Normal, 1 = Horizontally mirrored)
            uint8_t yFlip      : 1; // - Y flip (0 = Normal, 1 = Vertically mirrored)
            uint8_t bgPriority : 1; // - Priority (0 = Use attributes priority, 1 = BG and Window over OBJ)
        } __attribute__((packed)) attributes;
    } __attribute__((packed));

private:
    // PPU Modes =======================================================================================================

    // This enumeration defines the various operating modes of the Game Boy's PPU.
    // These modes indicate the current state of the PPU during the screen refresh cycle.
    enum class PPUMode {
        hBlank, // Mode 0 (Horizontal Blank): Waiting until the end of the current scanline.
        vBlank, // Mode 1 (Vertical Blank): Waiting until the next frame.
        oam,    // Mode 2 (OAM scan): Searching for sprites which overlap the current scanline.
        xfer,   // Mode 3 (Transfer/Drawing pixels): Sending pixels to the LCD controller.
    };

    void    setMode(PPUMode mode);  // Writes to lower two bits of the LCD Status Register
    uint8_t getMode() const;        // Reads from lower two bits of the LCD Status Register

    // PPU Mode Handlers ===============================================================================================

    // Mode 2: OAM Scan
    void handleModeOAM();                  // Mode 2: Scanline is being processed, OAM is locked.
    void scanOAM();                        // Searches OAM for sprites which overlap the current scanline.
    std::vector<Sprite> scanlineOAMBuffer; // Sprites on the current scanline (max 10).
    // Mode 3: Transfer
    void handleModeXfer();       // Mode 3: Scanline is being processed, both OAM and VRAM are locked.
    void runPixelFetcher();      // Fetches tile data and pushes it to the Pixel FIFO.
    void updateVideoBuffer();    // Pushes a pixel to the Pixel FIFO.
    // Mode 1: VBlank
    void handleModeVBlank(); // Mode 1: VBlank period, scanline 144-153, both OAM and VRAM are locked.
    // Mode 0: HBlank
    void handleModeHBlank(); // Mode 0: HBlank period, scanline 0-143, both OAM and VRAM are accessible.
    void calculateFPS();     // Calculates the FPS of the emulator and enforces frame timing.

private:
    // The FetcherState enum represents the different states of the Pixel Fetcher in the PPU,
    // which is responsible for fetching tile data and preparing it for the Pixel FIFO.
    enum FetcherState {
        getTileNumber,    // Fetching the tile number from the tile map.
        getTileDataLow,   // Fetching the low byte of the tile data.
        getTileDataHigh,  // Fetching the high byte of the tile data.
        sleep,            // Waiting (idle) state before pushing pixels to the FIFO.
        pushPixel,        // Pushing pixels to the FIFO.
    };

    // Pixel Fetcher State Handling ====================================================================================

    // Tile Number
    void handleFetcherStateTileNumber(); // Fetcher state for fetching tile number from tile map.
    void fetchSpriteTiles();             // Fetches sprite tiles from scanlineOAMBuffer.
    std::vector<Sprite> fetchedSprites;  // OAM entries fetched for the current scanline during pipeline (max 3).
    void fetchWindowTile();              // Fetches the window tile for the current scanline.
    // Tile Data
    void handleFetcherStateTileDataLow();  // Fetcher state for fetching low byte of tile data.
    void handleFetcherStateTileDataHigh(); // Fetcher state for fetching high byte of tile data.
    void fetchSpriteData(uint8_t offset);  // Fetches sprite tile data for the current scanline (0 = low, 1 = high).
    // Sleep
    void handleFetcherStateSleep(); // Fetcher state for waiting before pushing data to FIFO.
    // Push Pixel
    void handleFetcherStatePush();                                                  // Fetcher state for pushing pixels to FIFO.
    bool pushedToFIFO();                                                            // Pushes a pixel to the Pixel FIFO.
    uint32_t fetchSpritePixels(uint8_t bitPos, uint32_t color, uint8_t bgColorIdx); // Fetches sprite pixels for the FIFO

    // This struct encapsulates various state information and data storage for managing the flow of pixel data in
    // the PPU's pixel fetching process. Although it doesn't totally emulate the real hardware, whose documentation
    // is scarce, it does a good job of replicating the behavior of the PPU. The original hardware apparently uses
    // two queues for storing pixel data: one for the BG/W and one for sprites. The FIFO queue in this struct does
    // away with the need for two separate queues by directly mixing the BG/W and sprite pixel data into a single
    // queue based on selection and/or drawing priority information (see methods related to pushing pixels).
    struct PixelFIFO {
        FetcherState fetcherState = getTileNumber; // Current state of the Pixel Fetcher
        uint8_t pushedX      = 0x00;               // X-coordinate of the last pixel pushed to the video buffer
        uint8_t lyX          = 0x00;               // X-coordinate of the current scanline
        uint8_t fetcherX     = 0x00;               // Internal X-position counter for the Pixel Fetcher
        uint8_t bgwTileY     = 0x00;               // Used to calculate the current vertical tile being drawn by scanline
        uint8_t bgwTileX     = 0x00;               // Used to calculate the current horizontal tile being drawn by scanline
        uint8_t tileY        = 0x00;               // Used to find the correct vertical line we're on in the tile to get the tile data
        uint8_t fifoX        = 0x00;               // X-coordinate of the current pixel in the FIFO
        std::array<uint8_t, 3> bgwFetchData;       // Tile data fetched for BG/W (number, data low, data high)
        std::array<uint8_t, 6> oamFetchData;       // Tile data fetched for sprites: 3 sprites * 2 (data low & high)
        std::queue<uint32_t> fifo;                 // FIFO queue (a buffer holding pixel data for rendering)
    } pixelFifo;

    uint8_t windowLineCounter = 0x00; // Similar to LY, counts when the window is visible on the current scanline

private:
    // Useful constants.
    static constexpr uint16_t SCANLINES_PER_FRAME = 154;    // Scanlines in a single frame (i.e., LY = 0-153)
    static constexpr uint16_t DOTS_PER_SCANLINE   = 456;    // PPU clock cycles to process a single scanline

private:
    // Memory areas
    std::array<uint8_t, 0x2000> vram; // Video RAM (tile data storage from $8000-97FF)
    std::array<Sprite , 0x0028> oam;  // Object Attribute Memory stores sprite data (0x28=40 sprites, 4 bytes each)

private:
    uint8_t readVRAM(uint16_t addr);
    void    writeVRAM(uint16_t addr, uint8_t data);
    uint8_t readOAM(uint16_t addr);
    void    writeOAM(uint16_t addr, uint8_t data);

private:
    // Devices connected to the PPU
    Cartridge* cartridge;         // Pointer to the cartridge.
    LCD* lcd;                     // Pointer to the LCD controller.
    InterruptHandler* intHandler; // Interrupt handler for triggering STAT interrupts.
};

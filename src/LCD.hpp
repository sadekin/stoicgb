#pragma once

#include "common.hpp"
#include "InterruptHandler.hpp"
#include "Palette.hpp"

class DMA; // To prevent circular inclusions
class PPU; // To prevent circular inclusions

class LCD {
    friend class PPU;
    friend class UI;

public:
    LCD(DMA* dma, InterruptHandler* ih);
    ~LCD();

public:
    void    init();
    uint8_t read(uint16_t addr) const;
    void    write(uint16_t addr, uint8_t data);

public:
    void ConnectPPU(PPU* n) { ppu = n; }
    void ConnectDMA(DMA* n) { dma = n; }

private:
    // These registers control various aspects of the LCD's operation, including what is displayed,
    // how it is scrolled, and the colors used for rendering sprites and backgrounds.
    uint8_t lcdControl = 0x00; // LCD Control Register                                      (0xFF40)
    uint8_t lcdStatus  = 0x00; // LCD Status Register (STAT)                                (0xFF41)
    uint8_t scrollY    = 0x00; // Background viewport Y position                            (0xFF42)
    uint8_t scrollX    = 0x00; // Background viewport X position                            (0xFF43)
    uint8_t ly         = 0x00; // Current horizontal line (scanline) being processed        (0xFF44)
    uint8_t lyCompare  = 0x00; // Used for LY comparison, triggers interrupt if matched     (0xFF45)
    /*      dma                // For rapidly transferring data to OAM (see DMA.hpp)        (0xFF46)*/
    uint8_t bgp        = 0x00; // Color palette for background and window tiles             (0xFF47)
    uint8_t obp0       = 0x00; // Color palette for sprites using palette 0                 (0xFF48)
    uint8_t obp1       = 0x00; // Color palette for sprites using palette 1                 (0xFF49)
    uint8_t wy         = 0x00; // Window Y position                                         (0xFF4A)
    uint8_t wx         = 0x00; // Window X position + 7                                     (0xFF4B)

    // This enumeration defines bit flags for the LCD Control register (0xFF40).
    // Each flag corresponds to a specific bit in the register, controlling different aspects
    // of the LCD's operation and rendering behavior in the Game Boy system.
    // See: https://gbdev.io/pandocs/LCDC.html#ff40--lcdc-lcd-control
    enum LCDControlFlag {
        bgwEnable         = 1 << 0, // Controls display of the background & window   (0 = off; 1 = on)
        objEnable         = 1 << 1, // Controls display of sprites (objects)         (0 = off; 1 = on)
        objHeight         = 1 << 2, // Selects height of sprites                     (0 = 8 x 8; 1 = 8 x 16)
        bgTileMapArea     = 1 << 3, // Selects which bg tile map is displayed        (0 = 9800–9BFF; 1 = 9C00–9FFF)
        bgwTileDataArea   = 1 << 4, // Selects memory area used for bg&w tile data   (0 = 8800–97FF; 1 = 8000–8FFF)
        windowEnable      = 1 << 5, // Controls display of window layer              (0 = off; 1 = on)
        windowTileMapArea = 1 << 6, // Selects which window tile map is displayed    (0 = 9800–9BFF; 1 = 9C00–9FFF)
        lcdPPUEnable      = 1 << 7, // Controls operation of LCD & PPU               (0 = off; 1 = on)
    };

    uint16_t readLCDC(LCDControlFlag f) const; // Method for interpreting LCDC bits.

    // Enum class representing the LCD Status (STAT) register interrupt sources.
    // Each enumerator corresponds to a bit in the STAT register that enables a specific interrupt.
    // See: https://gbdev.io/pandocs/STAT.html#ff41--stat-lcd-status,
    //      https://gbdev.io/pandocs/Rendering.html#ppu-modes, and
    //      https://gbdev.io/pandocs/Interrupt_Sources.html#int-48--stat-interrupt
    enum class LCDStatusInterrupt {
        hBlank = (1 << 3), // Mode 0:  HBlank interrupt
        vBlank = (1 << 4), // Mode 1:  VBlank interrupt
        oam    = (1 << 5), // Mode 2:     OAM interrupt
        lyc    = (1 << 6), // LCY=LY: LCY==LY interrupt
    };

    bool interruptEnabled(LCD::LCDStatusInterrupt src) const; // Method for checking if a STAT interrupt is enable
    void incrementLY(); // Increments LY register, checks for LY=LYC condition, and triggers an interrupt if enable.
    bool windowIsVisible() const; // Checks if the window is visible on the current scanline.

private:
    // Arrays representing the color palettes for the background and sprites in RGBA format.
    // The default colors are white (transparent for sprites), light gray, dark gray, and black,
    // used for grayscale display emulation.
    // TODO: Implement support for changing the color palettes during gameplay.
    std::array<uint32_t, 4> defaultPalette = grayScalePalette; // Default Palette
    std::array<uint32_t, 4> bgPalette      = grayScalePalette; // Background Palette
    std::array<uint32_t, 4> obj0Palette    = grayScalePalette; // Sprite Palette 0
    std::array<uint32_t, 4> obj1Palette    = grayScalePalette; // Sprite Palette 1

    // Helper function to update the palettes bgPalette, obj0Palette, and obj1Palette based on the given palette data
    void updatePalette(uint8_t data, uint8_t palette);

private:
    static constexpr uint16_t Y_RESOLUTION = 144; // Vertical resolution of the GB screen in pixels
    static constexpr uint16_t X_RESOLUTION = 160; // Horizontal resolution of the GB screen in pixels

private:
    DMA* dma;                     // Pointer to a DMA object used for DMA transfers to the OAM (0xFF46, write-only)
    PPU* ppu = nullptr;           // Pointer to the PPU object
    InterruptHandler* intHandler; // For requesting STAT interrupts
};
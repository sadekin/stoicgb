#include "LCD.hpp"
#include "DMA.hpp"

LCD::LCD(DMA* dma, InterruptHandler* ih)
: dma(dma)
, intHandler(ih) {}

LCD::~LCD() = default;

/**
 * Reads from LCD Registers.
 *
 * @param addr The address to read from.
 * @return The data at the given address.
 */
uint8_t LCD::read(uint16_t addr) const {
    switch (addr) {
        case 0xFF40: return lcdControl;
        case 0xFF41: return lcdStatus | 0x80; // Bit 7 is unused
        case 0xFF42: return scrollY;
        case 0xFF43: return scrollX;
        case 0xFF44: return ly;
        case 0xFF45: return lyCompare;
        case 0xFF46: return dma->addrUpperByte; // Returns the last written value to the DMA register
        case 0xFF47: return bgp;
        case 0xFF48: return obp0;
        case 0xFF49: return obp1;
        case 0xFF4A: return wy;
        case 0xFF4B: return wx;
        default:     return 0xFF;
    }
}

/**
 * Writes to LCD Registers.
 *
 * @param addr The address to write to.
 * @param data The data to write.
 */
void LCD::write(uint16_t addr, uint8_t data) {
    switch (addr) {
        case 0xFF40: lcdControl = data; break;
        case 0xFF41: lcdStatus  = data; break;
        case 0xFF42: scrollY    = data; break;
        case 0xFF43: scrollX    = data; break;
        case 0xFF44:                    break; // LY is read-only; do nothing
        case 0xFF45: lyCompare  = data; break;
        case 0xFF46: dma->startTransfer(data); break; // DMA transfer (see DMA::startTransfer())
        case 0xFF47:
            updatePalette(data, 0);
            bgp = data;
            break;
        case 0xFF48:
            // The lower two bits are ignored because color index 0 is transparent for sprites.
            // See: https://gbdev.io/pandocs/Palettes.html#ff48ff49--obp0-obp1-non-cgb-mode-only-obj-palette-0-1-data
            updatePalette(data & 0b11111100, 1);
            obp0 = data;
            break;
        case 0xFF49:
            updatePalette(data & 0b11111100, 2);
            obp1 = data;
            break;
        case 0xFF4A: wy = data; break;
        case 0xFF4B: wx = data; break;
        default:                break;
    }
}

void LCD::init() {
    lcdControl = 0x91; // LCD Control Register                                      (0xFF40)
    lcdStatus  = 0x80; // LCD Status Register (STAT)                                (0xFF41)
    scrollY    = 0x00; // Background viewport Y position                            (0xFF42)
    scrollX    = 0x00; // Background viewport X position                            (0xFF43)
    ly         = 0x00; // Current horizontal line (scanline) being processed        (0xFF44)
    lyCompare  = 0x00; // Used for LY comparison, triggers interrupt if matched     (0xFF45)
    /*      dma                // For rapidly transferring data to OAM (see DMA.hpp)        (0xFF46)*/
    bgp        = 0xFC; // Color palette for background and window tiles             (0xFF47)
    obp0       = 0xFF; // Color palette for sprites using palette 0                 (0xFF48)
    obp1       = 0xFF; // Color palette for sprites using palette 1                 (0xFF49)
    wy         = 0x00; // Window Y position                                         (0xFF4A)
    wx         = 0x00; // Window X position + 7                                     (0xFF4B)
}

/**
 * Updates the color palette for the background, window, or sprites.
 *
 * @param data The new palette data to use.
 * @param palette The palette to update (0 = bgp, 1 = obp0, 2 = obp1).
 */
void LCD::updatePalette(uint8_t data, uint8_t palette) {
    std::array<uint32_t, 4>* pColors = &bgPalette;
    if (palette == 1)
        pColors = &obj0Palette;
    else if (palette == 2)
        pColors = &obj1Palette;

    for (int i = 0; i < 4; i++) {
        uint8_t colorIdx = (data >> (2 * i)) & 0b11;
        (*pColors)[i] = defaultPalette[colorIdx];
    }
}

/**
 * Interprets each bit of the LCD Control Register (see definition of LCD::LCDControlFlag enum).
 *
 * @param f LCD Control Flag mask for interpreting the LCD Control register.
 * @return Interpretation of the given flag bit (0/1 for bool flags, 0x8800/0x8000 for tile data area, etc.)
 */
uint16_t LCD::readLCDC(LCD::LCDControlFlag f) const {
    switch (f) {
        case bgwEnable:
        case objEnable:
        case windowEnable:
        case lcdPPUEnable:
            return (lcdControl & f) ? 1 : 0;
        case objHeight:
            return (lcdControl & f) ? 16 : 8;
        case bgTileMapArea:
        case windowTileMapArea:
            return (lcdControl & f) ? 0x9C00 : 0x9800;
        case bgwTileDataArea:
            return (lcdControl & f) ? 0x8000 : 0x8800;
    }
}

/**
 * Increments the LY register (which tracks the current scanline being drawn).
 * It also manages the window line counter if the window is visible on the current scanline.
 * When LY matches LYC (the value LY is compared against), it sets the Coincidence flag in the LCD Status
 * (STAT) Register and triggers an interrupt if the LY=LYC interrupt is enable. This function should be called
 * at the end of each scanline, which occurs every 456 T-Cycles (or 114 CPU M-Cycles).
 */
void LCD::incrementLY() {
    // The Window Line Counter is a fetcher-internal variable which is incremented
    // each time a scanline had any window pixels on it and reset when entering VBlank mode.
    if (windowIsVisible() && (wy <= ly && ly < wy + Y_RESOLUTION))
        ppu->windowLineCounter++;

    // Increment the LY register to move to the next scanline.
    ly++;

    // Check if LY now matches the LY Compare value (LYC), and set or clear the LY=LYC flag accordingly.
    if (ly == lyCompare) {
        // Set the LY=LYC (Coincidence) flag in the lcdStatus register to indicate a match.
        lcdStatus |= (1 << 2);

        // If the LY=LYC interrupt is enable, request an LCD STAT interrupt.
        if (interruptEnabled(LCDStatusInterrupt::lyc))
            intHandler->irq(InterruptHandler::lcdStat);
    } else {
        // Clear the LY=LYC flag in the STAT register as LY does not match LYC.
        lcdStatus &= ~(1 << 2);
    }
}

/**
 * Checks if the window is visible on the screen.
 * See: https://gbdev.io/pandocs/Scrolling.html#ff4aff4b--wy-wx-window-y-position-x-position-plus-7
 *
 * @return True if the window is visible, false otherwise.
 */
bool LCD::windowIsVisible() const {
    return
        readLCDC(windowEnable) &&
        (0 <= wx && wx < X_RESOLUTION + 7) &&
        (0 <= wy && wy < Y_RESOLUTION);
}

/**
 * Checks if the given STAT interrupt source is enable.
 * Bits 3-6 of the LCD Status Register (STAT) control STAT interrupts.
 * (see LCD::LCDStatusInterrupt enum).
 *
 * @param src The STAT interrupt source to check.
 * @return True if the interrupt is enable (STAT bit on), false otherwise.
 */
bool LCD::interruptEnabled(LCD::LCDStatusInterrupt src) const {
    return lcdStatus & static_cast<uint8_t>(src);
}

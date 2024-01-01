#include "PPU.hpp"

PPU::PPU(Cartridge* c, LCD* l, InterruptHandler* ih)
: cartridge(c)
, lcd(l)
, intHandler(ih)
, pixelFifo()
, fetchedSprites()
, oam()
, vram()
, videoBuffer()
, scanlineOAMBuffer() {
    setMode(PPUMode::oam);
    std::fill(oam.begin(), oam.end(), Sprite());
    std::fill(vram.begin(), vram.end(), 0);
    std::fill(videoBuffer.begin(), videoBuffer.end(), 0);
}

PPU::~PPU() = default;

/**
 * Reads a byte of data from OAM or VRAM.
 *
 * @param addr The address to read from.
 * @return The byte of data at the specified address.
 */
uint8_t PPU::read(uint16_t addr) {
    if (addr >= 0x8000 && addr <= 0x9FFF) // VRAM
        return readVRAM(addr);
    else if (addr >= 0xFE00 && addr <= 0xFE9F) // OAM
        return readOAM(addr);
    else
        throw std::runtime_error("Invalid PPU read at address: " + std::to_string(addr));
}

/**
 * Writes a byte of data to OAM or VRAM.
 *
 * @param addr The address to write to.
 * @param data The byte of data to write to the specified address.
 */
void PPU::write(uint16_t addr, uint8_t data) {
    if (addr >= 0x8000 && addr <= 0x9FFF) // VRAM
        writeVRAM(addr, data);
    else if (addr >= 0xFE00 && addr <= 0xFE9F) // OAM
        writeOAM(addr, data);
    else
        throw std::runtime_error("Invalid PPU write at address: " + std::to_string(addr));
}

/**
 * Reads a byte of data from Video RAM (VRAM).
 * VRAM is used to store graphical data, such as tile sets and background maps.
 * This function reads a byte from VRAM at the specified address.
 *
 * VRAM is 8 KB in size and is located in the memory range 0x8000 - 0x9FFF.
 * 0x8000 - 0x9FFF : 8 KB Video RAM (VRAM) - In CGB mode, switchable bank 0/1
 *      0x8000 - 0x97FF : CHR RAM
 *      0x9800 - 0x9BFF : BG Map 1
 *      0x9C00 - 0x9FFF : BG Map 2
 *
 * @param addr The VRAM address from which to read. This address is offset by the VRAM start address (0x8000).
 * @return The byte of data read from the specified VRAM address.
 */
uint8_t PPU::readVRAM(uint16_t addr) {
    return vram[addr - 0x8000];
}

/**
 * Writes a byte of data to Video RAM (VRAM).
 * VRAM is used to store graphical data, such as tilesets and background maps.
 * This function writes a byte to VRAM at the specified address.
 *
 * @param addr The VRAM address where the data should be written.
 *             This address is offset by the VRAM start address (0x8000).
 * @param data The byte of data to write to the specified VRAM address.
 */
void PPU::writeVRAM(uint16_t addr, uint8_t data) {
    vram[addr - 0x8000] = data;
}

/**
 * Reads a byte of data from Object Attribute Memory (OAM).
 * OAM is used to store sprite attributes such as position, tile index, and flags.
 * This function reads a byte from OAM at the specified address.
 *
 * OAM is 160 bytes in size and is located in the memory range 0xFE00 - 0xFE9F.
 *
 * @param addr The address in OAM to read from. Addresses are offset by the OAM start address (0xFE00).
 * @return The byte of data read from the specified OAM address.
 */
uint8_t PPU::readOAM(uint16_t addr) {
    auto* p = reinterpret_cast<uint8_t*>(&oam);
    return p[addr - 0xFE00];
}

/**
 * Writes a byte of data to Object Attribute Memory (OAM).
 * OAM is used to store sprite attributes such as position, tile index, and flags.
 * This function writes a byte to OAM at the specified address.
 *
 * @param addr The address in OAM to write to. Addresses are offset by the OAM start address (0xFE00).
 * @param data The byte of data to be written to the specified OAM address.
 */
void PPU::writeOAM(uint16_t addr, uint8_t data) {
    auto* p = reinterpret_cast<uint8_t*>(&oam);
    p[addr - 0xFE00] = data;
}

/**
 * Sets the current LCD Mode as defined by the lower two bits of the LCD Status Register.
 *
 * @param mode The new LCD Mode to set (see LCD::PPUMode enum).
 */
void PPU::setMode(PPUMode mode) {
    lcd->lcdStatus &= ~0b11;
    lcd->lcdStatus |= static_cast<uint8_t>(mode);
}

/**
 * Reads the current LCD Mode as defined by the lower two bits of the LCD Status Register.
 *
 * @return The current LCD Mode (see LCD::PPUMode enum).
 */
uint8_t PPU::getMode() const {
    return lcd->lcdStatus & 0b11;
}

/**
 * Advances the PPU state by one tick (one dot on the screen).
 * The Game Boy PPU operates on a cycle that processes scan-lines to render frames.
 * This function manages the state transitions and processing for each mode of the PPU.
 */
void PPU::tick() {
    dots++;
    switch (getMode()) {
        case static_cast<uint8_t>(PPUMode::oam):    handleModeOAM();    break;
        case static_cast<uint8_t>(PPUMode::xfer):   handleModeXfer();   break;
        case static_cast<uint8_t>(PPUMode::hBlank): handleModeHBlank(); break;
        case static_cast<uint8_t>(PPUMode::vBlank): handleModeVBlank(); break;
    }
}

// PPU State Machine ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * GAME BOY PPU CYCLE OVERVIEW:
 * See: https://gbdev.io/pandocs/Rendering.html?search=#ppu-modes and
 *      https://github.com/ISSOtm/pandocs/blob/rendering-internals/src/Rendering_Internals.md
 *
 * Mode 2: OAM Scan (80 dots)
 *          - During this mode, the PPU is scanning the OAM for OBJs (sprites) to be rendered.
 *          - VRAM, CGB palettes, and OAM (except by DMA) are inaccessible.
 *
 * Mode 3: Drawing Pixels (172-289 dots, depending on sprite count and position)
 *          - Here, the PPU is drawing pixels to the LCD.
 *          - Access to VRAM and CGB palettes is blocked.
 *
 * Mode 0: Horizontal Blank (HBlank) (87-204 dots, until reaching 456 dots per line)
 *          - The PPU has finished drawing a scanline and the CPU can access VRAM and OAM.
 *
 * Mode 1: Vertical Blank (VBlank) (4560 dots, for 10 scan-lines)
 *          - The PPU has finished drawing all visible scan-lines and enters VBlank.
 *          - This period spans scan-lines 144 to 153.
 *          - During VBlank, the CPU can access all graphics memory without restrictions.
 *          - An interrupt can be triggered at the start of VBlank.
 *
 * A complete frame consists of 70224 dots, which, at a rate of 59.7 frames per second,
 * corresponds to the entire screen being refreshed.
 *
 * Note: The scanline counter 'LY' ranges from 0 to 153. Scan-lines 0 to 143 are visible lines,
 * while 144 to 153 correspond to VBlank. The PPU cycles through modes 2, 3, and 0 in each
 * of the 144 visible scan-lines, and mode 1 during VBlank.
 *
 * Each scanline lasts 456 dots (clock cycles), which includes modes 2, 3, and 0.
 * The duration of each mode can vary slightly depending on sprite rendering.
 *
 * Note: `A "dot" is the unit of time within the PPU. One "dot" is one 4 MHz cycle, i.e. a unit of time equal
 * to 1 ∕ 4194304 of a second. The duration of one "dot" is independent of CGB double speed.'
 * - https://github.com/ISSOtm/pandocs/blob/rendering-internals/src/Rendering_Internals.md
 */

/**
 * MODE 2: Handles the Object Attribute Memory (OAM) Mode of the PPU.
 * In this mode, the PPU scans the OAM to fetch sprite data for the current scanline.
 * OAM Mode is the first stage in rendering a scanline and lasts for exactly 80 clock cycles (dots/ticks).
 */
void PPU::handleModeOAM() {
    // On the first tick of OAM Mode, initialize the process of loading sprite data for the current scanline.
    // This involves scanning the OAM to determine which sprites are visible on the current scanline.
    if (dots == 1)
        scanOAM();

    // OAM Mode lasts for 80 dots.
    if (dots == 80) {
        // Transition from OAM Mode to Transfer (Xfer) Mode.
        // In Transfer Mode, the PPU fetches both background and sprite data for rendering the scanline.
        setMode(PPUMode::xfer);

        // Reset the pixel fetcher state and coordinates for fetching operations in the new scanline.
        // The pixel fetcher is responsible for retrieving pixel data from VRAM & OAM and feeding it to the Pixel FIFO.
        pixelFifo.fetcherState = getTileNumber; // Begin fetching tile data for the background/window.
        pixelFifo.lyX      = 0x00;     // Reset the X-coordinate for the scanline.
        pixelFifo.fetcherX = 0x00;     // Reset the X-coordinate for fetch operations.
        pixelFifo.pushedX  = 0x00;     // Reset the X-coordinate for pixels pushed to the FIFO.
        pixelFifo.fifoX    = 0x00;     // Reset the X-coordinate within the FIFO.
    }
}

/**
 * Loads sprites that are visible on the current scanline into a buffer for processing.
 * This function examines the sprite attributes in the Object Attribute Memory (OAM)
 * to determine which sprites are visible on the current scanline and should be considered
 * for rendering. It respects the Game Boy hardware limitation of a maximum of 10 sprites
 * per scanline and sorts visible sprites by their X-coordinate.
 */
void PPU::scanOAM() {
    // Clear the OAM buffer for the current scanline.
    scanlineOAMBuffer.clear();

    // Determine the height of sprite: Either 8 pixels (normal) or 16 pixels (tall sprites).
    uint8_t spriteHeight = lcd->readLCDC(LCD::objHeight);

    // Scan OAM to find sprites that are visible on the current scanline.
    for (auto& sprite : oam) {
        if (sprite.yPos == 0 || sprite.yPos >= LCD::Y_RESOLUTION + 16) // Sprite hidden (real y = yPos - 16)
            continue;

        if (sprite.yPos <= lcd->ly + 16 && lcd->ly + 16 < sprite.yPos + spriteHeight) { // Visible on current scanline
            // Sort by X-coordinate to ensure sprites are drawn in the correct order.
            // If X-coordinates are equal, the sprite located first in OAM has priority.
            // See https://gbdev.io/pandocs/OAM.html#selection-priority
            auto it = std::upper_bound(
                    scanlineOAMBuffer.begin(),
                    scanlineOAMBuffer.end(),
                    sprite,
                    [](const Sprite& a, const Sprite& b) { return a.xPos < b.xPos; }
            );
            scanlineOAMBuffer.insert(it, sprite);

            if (scanlineOAMBuffer.size() >= 10) // Max 10 sprites per scanline
                break;

        }
    }
}

/**
 * MODE 3: Handles the Transfer (Xfer) Mode of the PPU, during which the PPU is
 * actively drawing the GB screen's contents - background, window, and sprites
 * (sending pixels to the LCD) This mode follows OAM and lasts between 172 and 289 dots.
 * See: https://gbdev.io/pandocs/Rendering.html#mode-3-length
 */
void PPU::handleModeXfer() {
    // Only run Pixel Fetcher every other tick to simulate the PPU's clock cycle behavior.
    // "The Game Boy CPU and PPU run in parallel. The 4.2 MHz master clock is also the dot clock.
    // It's divided by 2 to form the PPU's 2.1 MHz memory access clock, and divided by 4 to form
    // a multiphase 1.05 MHz clock used by the CPU." - tepples
    // See https://forums.nesdev.org/viewtopic.php?f=20&t=17754&p=225009#p225009
    if (!(dots & 1))
        runPixelFetcher();

    // Push pixels to the video buffer for rendering.
    updateVideoBuffer();

    // Check if the X-coordinate of the last pixel pushed to the FIFO has reached the screen's width.
    // This indicates that the visible portion of the current scanline has been processed.
    if (pixelFifo.pushedX >= LCD::X_RESOLUTION) {
        // Clear the FIFO to prepare for processing the next scanline.
        while (!pixelFifo.fifo.empty())
            pixelFifo.fifo.pop();

        // Transition to the Horizontal Blank (H-Blank) mode.
        // This occurs when the PPU has finished drawing a scanline and is "resting" before starting the next one.
        setMode(PPUMode::hBlank);

        // Check if the H-Blank interrupt is enable in the LCD Status register.
        // If enabled, triggered the LCD STAT interrupt to inform the CPU that H-Blank has started.
        if (lcd->interruptEnabled(LCD::LCDStatusInterrupt::hBlank))
            intHandler->irq(InterruptHandler::InterruptType::lcdStat);
    }
}

/**
 * Pushes pixel data from the Pixel FIFO to the video buffer for rendering.
 * This function processes the pixel data in the FIFO, ensuring it's correctly positioned on the screen.
 */
void PPU::updateVideoBuffer() {
    if (pixelFifo.fifo.size() > 8) { // Has enough pixels to render part of the scanline
        uint32_t pixelData = pixelFifo.fifo.front();
        pixelFifo.fifo.pop();

        // The scrollX register value affects the starting point of the visible region on the scanline.
        // 'lyX' is used to track the current position on the scanline being processed.
        // Pixels are only pushed to the video buffer if they fall within the visible region of the scanline.
        if (pixelFifo.lyX >= lcd->scrollX % 8) {
            // Calculate the index in the video buffer where the pixel data should be placed.
            // Note that we are using row-major order to store the pixel data in the video buffer,
            // where the column='pushedX', row='ly', and width='X-RESOLUTION' (https://stackoverflow.com/a/2151141).
            videoBuffer[pixelFifo.pushedX + (lcd->ly * LCD::X_RESOLUTION)] = pixelData;

            // Increment the pushedX counter, indicating the next position for the subsequent pixel on this scanline.
            pixelFifo.pushedX++;
        }

        // Increment the lyX counter to proceed to the next horizontal position on current scanline.
        pixelFifo.lyX++;
    }
}

/**
 * MODE 0: Handles the Horizontal Blank (HBlank) Mode of the PPU.
 * HBlank occurs after a scanline is finished being drawn, allowing CPU to access VRAM and OAM.
 * This mode lasts until the remaining time of the 456-tick scanline cycle.
 */
void PPU::handleModeHBlank() {
    if (dots == DOTS_PER_SCANLINE) {
        lcd->incrementLY(); // Increment LY for the next scanline.

        if (lcd->ly >= LCD::Y_RESOLUTION) { // Check if all visible scan-lines are processed.
            setMode(PPUMode::vBlank); // Transition to VBlank mode after all visible lines are drawn.

            intHandler->irq(InterruptHandler::vBlank); // Request a VBlank interrupt.

            // Request LCD STAT interrupt if VBlank interrupt is enabled.
            if (lcd->interruptEnabled(LCD::LCDStatusInterrupt::vBlank))
                intHandler->irq(InterruptHandler::lcdStat);

            // Calculate and print FPS for monitoring performance.
            calculateFPS();
        } else {
            // If still within the visible scan-lines, switch back to OAM mode.
            setMode(PPUMode::oam);

            if (lcd->interruptEnabled(LCD::LCDStatusInterrupt::oam))
                intHandler->irq(InterruptHandler::lcdStat);
        }
        dots = 0; // Reset tick counter for the next line.
    }
}

// Constants for frame timing control.
constexpr int TARGET_FPS = 60;                               // Target frame rate in frames per second.
constexpr int TARGET_FRAME_DURATION = 1000 / TARGET_FPS - 1; // Target duration of a single frame.

// Variables for FPS calculation and frame timing.
static uint64_t currFrameDuration = 0;
static uint64_t currTimestamp     = 0;
static uint64_t prevTimestamp     = 0;
static uint64_t fpsCalcStartTime  = 0;
static uint64_t framesThisSecond  = 0;
static uint32_t fps               = 0;

/**
 * Computes and logs the frames per second (FPS) and enforces frame timing to maintain roughly 60 fps.
 */
void PPU::calculateFPS() {
    framesRendered++; // Increment total frame count.

    // Calculate time taken for the current frame.
    currTimestamp = SDL2Timing::getTicks();
    currFrameDuration = currTimestamp - prevTimestamp;

    if (currFrameDuration < TARGET_FRAME_DURATION)
        SDL2Timing::delay(std::chrono::milliseconds(TARGET_FRAME_DURATION - currFrameDuration)); // Delay to maintain 60 fps.

    if (currTimestamp - fpsCalcStartTime >= 1000) { // Update FPS every second (1000 ms).
        fps = framesThisSecond;           // Record FPS for logging.
        fpsCalcStartTime = currTimestamp; // Reset the stat time for the next second.
        framesThisSecond = 0;             // Resent the frame count for the next second.
        printf("FPS: %d\n", fps);         // Log FPS.

        if (cartridge->needsToSave())     // Save the cartridge if it needs to be saved.
            cartridge->save();
    }
    framesThisSecond++;                     // Increment frame count for FPS calculation.
    prevTimestamp = SDL2Timing::getTicks(); // Update timestamp for the next frame.
}

/**
 * MODE 1: Handles the Vertical Blank (VBlank) Mode of the PPU (waiting until next frame).
 * VBlank occurs after all visible lines have been drawn and represents a period where
 * the screen is not being updated, lasting 4560 ticks:
 * 10 scan-lines from LY = 144 to LY = 153, each scanline taking 456 ticks => 10 * 456 = 4560 dots.
 */
void PPU::handleModeVBlank() {
    if (dots == DOTS_PER_SCANLINE) { // Each scanline takes 456 dots
        lcd->incrementLY(); // Increment the LY register (current scanline).

        // If all scan-lines are processed, reset LY and switch to OAM mode.
        if (lcd->ly >= SCANLINES_PER_FRAME) { // 154 scan-lines per frame.
            setMode(PPUMode::oam);            // Transition back to OAM mode.

            if (lcd->interruptEnabled(LCD::LCDStatusInterrupt::oam))
                intHandler->irq(InterruptHandler::lcdStat);

            lcd->ly = 0;           // Reset scanline counter after a full frame.
            windowLineCounter = 0; // Reset window line counter after a full frame.
        }
        dots = 0; // Reset tick counter for the next scanline.
    }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


// Pixel Fetcher ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/**
 * Executes the current state of the fetcher pipeline in the PPU.
 * The fetcher pipeline sequentially retrieves and processes tile data, preparing it for the Pixel FIFO.
 */
void PPU::runPixelFetcher() {
    switch (pixelFifo.fetcherState) {
        case getTileNumber:   handleFetcherStateTileNumber();   break;
        case getTileDataLow:  handleFetcherStateTileDataLow();  break;
        case getTileDataHigh: handleFetcherStateTileDataHigh(); break;
        case sleep:           handleFetcherStateSleep();        break;
        case pushPixel:       handleFetcherStatePush();         break;
    }
}

// Tile ================================================================================================================
/**
 * Handles the tile state of the pixel fetcher in the PPU.
 * In this state, the fetcher retrieves the tile number from the tile map for background/window rendering.
 * It also initiates sprite tile fetching if sprites are enable and present on the current scanline.
 */
void PPU::handleFetcherStateTileNumber() {
    // Check if background/window display is enable.
    if (lcd->readLCDC(LCD::bgwEnable)) {
        pixelFifo.bgwTileY = lcd->ly + lcd->scrollY;             // For getting the tile row
        pixelFifo.bgwTileX = pixelFifo.fetcherX + lcd->scrollX;  // For getting the tile column
        pixelFifo.tileY    = ((lcd->ly + lcd->scrollY) % 8) * 2; // Each vertical line of a tile is 2 bytes

        // Fetch the tile number from the tile map in memory.
        // The background is a 256x256 pixel map, with each tile being 8x8 (=64 total) pixels,
        // and each pixel being 2bpp (2 bits per pixel).
        // Therefore, the tile map is 32x32 tiles (256/8), with each tile being 16 bytes:
        // (2bpp * 64 pixels) / (8 bits per byte) = 16 bytes per tile.
        // So the address is calculated in row-major order as follows to map the multidimensional values to
        // a flat array structure (the VRAM) (see https://stackoverflow.com/a/2151141)
        pixelFifo.bgwFetchData[0] = readVRAM(
                lcd->readLCDC(LCD::bgTileMapArea) +         // Tile map start address
                (pixelFifo.bgwTileY / 8) * 32 +             // Tile row * 32 tiles per row
                ((pixelFifo.bgwTileX / 8) & 0x1F)           // Tile column (wraps around 32)
        );

        // If the window is visible, load the window tile data for the current scanline.
        if (lcd->windowIsVisible())
            fetchWindowTile();

        // This memory region uses signed bytes as tile identifiers.
        // See: https://gbdev.io/pandocs/Tile_Data.html#vram-tile-data
        if (lcd->readLCDC(LCD::bgwTileDataArea) == 0x8800) {
            pixelFifo.bgwFetchData[0] = static_cast<int8_t>(pixelFifo.bgwFetchData[0]);
            pixelFifo.bgwFetchData[0] += 128;
        }
    }

    // Reset the number of fetched sprite entries.
    fetchedSprites.clear();

    // If sprites are enable, load sprite tile data for overlapping sprites on the current scanline.
    if (lcd->readLCDC(LCD::objEnable))
        fetchSpriteTiles();

    // Transition to the next state to fetch the lower byte of tile data.
    pixelFifo.fetcherState = getTileDataLow;

    // Prepare to fetch the next tile after processing the current one.
    pixelFifo.fetcherX += 8; // Each tile is 8 pixels wide
}

// The window's X-coordinate is offset by 7 pixels.
// See: https://gbdev.io/pandocs/Scrolling.html#ff4aff4b--wy-wx-window-y-position-x-position-plus-7
static constexpr uint8_t WX_OFFSET = 7;

/**
 * Loads a window tile into the Pixel FIFO during the rendering process.
 *
 * This function is part of the PPU's pipeline and is responsible for loading
 * window tile data into the Pixel FIFO. The window is an optional overlay
 * that can be displayed above the background. This function calculates
 * which tile of the window should be loaded based on the current scanline
 * and the window's position.
 *
 * TODO: Super Mario Land pause screen is not properly centered.
 */
void PPU::fetchWindowTile() {
    // Check if fetcherX is within the window's horizontal range.
    // The window's effective X coordinate is wx - 7 pixels due to hardware behavior.
    if (lcd->wx <= pixelFifo.fetcherX + WX_OFFSET && pixelFifo.fetcherX + WX_OFFSET < lcd->wx + LCD::X_RESOLUTION) {
        // Check if the current scanline is within the window's vertical range.
        if (lcd->wy <= lcd->ly && lcd->ly < lcd->wy + LCD::Y_RESOLUTION) {
            pixelFifo.bgwTileY = windowLineCounter;
            pixelFifo.bgwTileX = pixelFifo.fetcherX + WX_OFFSET - lcd->wx;

            pixelFifo.bgwFetchData[0] = readVRAM(
                    lcd->readLCDC(LCD::windowTileMapArea) +         // Tile map start address
                    pixelFifo.bgwTileY / 8 * 32 +                   // Tile row * 32 tiles per row
                    ((pixelFifo.bgwTileX / 8) & 0x1F)               // Tile column (wraps around 32)
            );

            pixelFifo.tileY = (windowLineCounter % 8) * 2; // Set Y-coordinate of the pixel within the tile (0-15)
        }
    }
}

/**
 * Loads the sprite tiles that are to be considered for rendering on the current line.
 * This function checks the sprites that have been loaded as visible on the current scanline
 * and determines which of these should have their tile data fetched for possible rendering.
 * It considers sprite visibility based on their X-coordinate and the current X-coordinate of
 * the fetch operation.
 */
void PPU::fetchSpriteTiles() {
    for (auto &sprite : scanlineOAMBuffer) {
        // If three sprites have already been considered for this pixel, stop fetching more.
        if (fetchedSprites.size() >= 3)
            break;

        int spriteX = (sprite.xPos - 8) + (lcd->scrollX % 8); // Adjust X-coordinate for scrolling.

        // Check if the sprite's X-coordinate falls within the range of pixels being fetched.
        if ((pixelFifo.fetcherX <= spriteX && spriteX < pixelFifo.fetcherX + 8) || // Overlaps with current tile
            (pixelFifo.fetcherX <= spriteX + 8 && spriteX + 8 < pixelFifo.fetcherX + 8)) { // Overlaps with next tile
            // If the sprite is within the fetch range, add it to the array of fetched sprite entries.
            fetchedSprites.push_back(sprite);
        }
    }
}

// =====================================================================================================================


// Tile Data ===========================================================================================================
/**
 * Fetches the low byte of the background/window tile data from memory.
 * This byte determines the lower bit of the color index for each pixel in the current row of the tile.
 * In addition, this function initiates the fetch of sprite tile data if applicable.
 * The function is part of the PPU's pixel pipeline process during the tile data low fetch state.
 */
void PPU::handleFetcherStateTileDataLow() {
    // Fetch the low byte of the tile data from the calculated address.
    // This byte contains bit 0 of the color index for each pixel in the current row of the tile.
    pixelFifo.bgwFetchData[1] = readVRAM(
            lcd->readLCDC(LCD::bgwTileDataArea) +   // Tile data start address (0x8800 or 0x8000)
            (pixelFifo.bgwFetchData[0] * 16) +      // Tile number (0-255) * 16 bytes per tile
            pixelFifo.tileY                         // Current row within tile (0-7)
    );

    // Load sprite data associated with the lower bits if sprites are enable and overlap this tile.
    fetchSpriteData(0);

    // Transition to the next state to fetch the high byte of the tile data.
    pixelFifo.fetcherState = getTileDataHigh;
}

/**
 * Fetches the high byte of the background/window tile data from memory.
 * This byte determines the upper bit of the color index for each pixel in the current row of the tile.
 * Additionally, this function initiates the fetch of sprite tile data's higher bits if applicable.
 * The function is part of the PPU's pixel pipeline process during the tile data high fetch state.
 */
void PPU::handleFetcherStateTileDataHigh() {
    // Fetch the high byte of the tile data from memory.
    // This byte contains bit 1 of the color index for each pixel in the current row.
    pixelFifo.bgwFetchData[2] = readVRAM(
            lcd->readLCDC(LCD::bgwTileDataArea) +   // Tile data start address (0x8800 or 0x8000)
            (pixelFifo.bgwFetchData[0] * 16) +      // Tile index (0-255) * 16 bytes per tile
            (pixelFifo.tileY + 1)                   // Next row within tile (0-7)
    );

    // Load sprite data associated with the higher bits if sprites are enable and overlap this tile.
    fetchSpriteData(1);

    // Transition to the sleep state, where the fetcher waits before pushing data to the FIFO.
    pixelFifo.fetcherState = sleep;
}

/**
 * Loads sprite tile data into the Pixel FIFO's OAM fetch data buffer for up to three sprites.
 * This function fetches the sprite tile data from VRAM, tailored to the current scanline
 * and the specific attributes of the sprites. It handles up to three sprites in one cycle,
 * fetching both low and high bytes of the tile data for each sprite. The function is designed
 * to work with sprites of varying heights (8x8 or 8x16 pixels) and accounts for attributes like
 * vertical flipping.
 *
 * @param offset The byte offset within the tile data to fetch. This is 0 for the low byte and 1 for the high byte.
 *               The function processes two bytes per sprite (low and high) for up to three sprites.
 */
void PPU::fetchSpriteData(uint8_t offset) {
    uint8_t spriteHeight = lcd->readLCDC(LCD::objHeight); // Get the sprite height (8x8 or 8x16).

    for (int i = 0; i < fetchedSprites.size(); i++) {
        // Calculate the relative Y position within the sprite, accounting for two bytes per tile row.
        // This determines which row of the sprite tile to fetch.
        //
        // Note: The addition of 16 in the calculation accounts for the sprite's Y coordinate system
        // in the Game Boy hardware. In the Game Boy's graphics system, the sprite coordinates are
        // offset by 16 pixels vertically. This means that a sprite with a Y position of 0 is actually
        // displayed 16 pixels down from the top of the visible screen area.
        // See the image at https://gbdev.io/pandocs/OAM.html#byte-0--y-position
        uint8_t tileY = (lcd->ly - (fetchedSprites.at(i).yPos - 16)) * 2;

        // If the sprite is flipped vertically, calculate the offset from the bottom instead.
        if (fetchedSprites.at(i).attributes.yFlip)
            tileY = ((spriteHeight * 2) - 2) - tileY;

        uint8_t tileNum = fetchedSprites.at(i).tileNum;

        // In 8x16 mode, ensure the tile index is even as each sprite spans two tiles.
        if (spriteHeight == 16)
            tileNum &= ~1; // Clear least significant bit for even alignment.

        // Calculate the memory address in VRAM for fetching the sprite data.
        // This includes the base address (0x8000), the tile number, and the tile data offset (0=low or 1=high).
        pixelFifo.oamFetchData[(i * 2) + offset] = readVRAM(0x8000 + (tileNum * 16) + tileY + offset);
    }
}

// =====================================================================================================================


// Sleep ===============================================================================================================
/**
 * Handles the sleep state in the pixel fetching process.
 * During this state, the fetcher does not perform any operations and simply
 * transitions to the pushPixel state. This state acts as a brief pause between
 * fetching tile data and pushing pixels to the FIFO.
 */
void PPU::handleFetcherStateSleep() {
    // Transition from the sleep state to the pushPixel state.
    // In the pushPixel state, pixel data will be pushed into the FIFO for rendering.
    pixelFifo.fetcherState = pushPixel;
}

// =====================================================================================================================


// Push ================================================================================================================
/**
 * Pushes pixel data to the FIFO for rendering.
 * This function is part of the pixel pipeline fetch process, specifically
 * handling the state where the fetcher pushes pixel data to the FIFO,
 * which is then processed and eventually rendered to the display.
 */
void PPU::handleFetcherStatePush() {
    if (pushedToFIFO())
        pixelFifo.fetcherState = getTileNumber;
}

/**
 * Adds a row of pixel data to the FIFO from the current tile's fetched data.
 * This function handles the process of transforming tile data into pixel colors
 * and storing them in the FIFO for rendering. The FIFO acts as a buffer, holding
 * pixel data until it is ready to be displayed on the screen.
 *
 * The function calculates the x-coordinate adjusted for the current scroll position,
 * ensuring that only visible pixels are added to the FIFO. It processes each bit
 * in the fetched tile data to determine the color of each pixel based on the background
 * palette and adds the corresponding 32-bit color value to the FIFO.
 *
 * @return True if the pixel was successfully pushed to the FIFO, or false if the FIFO is full.
 */
bool PPU::pushedToFIFO() {
    // Check if the FIFO already contains more than 8 pixels, indicating it's full.
    if (pixelFifo.fifo.size() > 8)
        return false;

    // Calculate the x-coordinate adjusted for the current horizontal scroll position.
    // This is necessary to correctly align the fetched pixel data with the visible part of the background.
    int adjustedX = pixelFifo.fetcherX - (8 - (lcd->scrollX % 8));
    if (adjustedX < 0)
        return true; // Skip pixels that are off-screen due to horizontal scrolling.

    // Process each bit in the fetched tile data to determine the color index of each pixel.
    for (int bitPos = 7; bitPos >= 0; bitPos--) {
        uint8_t colorIdx = (!!(pixelFifo.bgwFetchData[2] & (1 << bitPos)) << 1) | // high bit from high tile data byte
                           (!!(pixelFifo.bgwFetchData[1] & (1 << bitPos)) << 0);  // low bit from low tile data byte

        // Determine the color from the background palette.
        uint32_t color = lcd->bgPalette[colorIdx];

        // If background/window display is disabled, use the default color (white=transparent for BG/W).
        if (!lcd->readLCDC(LCD::bgwEnable))
            color = lcd->bgPalette[0];

        // If sprite display is enable, fetch sprite pixels and override the background color if necessary.
        if (lcd->readLCDC(LCD::objEnable))
            color = fetchSpritePixels(bitPos, color, colorIdx);

        pixelFifo.fifo.push(color);
        pixelFifo.fifoX++;
    }

    return true; // Successfully added pixel data to the FIFO.
}

/**
 * Fetches the color value of sprite pixels for the current position in the Pixel FIFO.
 * This function iterates through the sprite entries fetched for the current scanline
 * and determines the appropriate sprite pixel color to display, taking into account
 * sprite priority and background color.
 *
 * @param bitPos The position within the 8-pixel span of the sprite data from which to fetch the color.
 * @param color The default color to be used if no sprite pixel overrides it.
 * @param bgColorIdx The color index of the background pixel at the current FIFO position.
 *
 * @return The color value for the current pixel, which might be the unchanged input color,
 *         or a new color based on the sprite's pixel data and attributes.
 */
uint32_t PPU::fetchSpritePixels(uint8_t bitPos, uint32_t color, uint8_t bgColorIdx) {
    // Loop over all fetched sprite entries to check for sprite pixel data at the current FIFO x position.
    for (int i = 0; i < fetchedSprites.size(); i++) {
        // Calculate the effective x position of the sprite on the screen, accounting for the scroll position.
        int spriteX = (fetchedSprites.at(i).xPos - 8) + (lcd->scrollX % 8);

        // If the sprite's right edge is to the left of the current FIFO position, it's not relevant.
        if (spriteX + 8 < pixelFifo.fifoX)
            continue;

        // Calculate the offset from the left edge of the sprite to the current FIFO x position.
        int offset = pixelFifo.fifoX - spriteX;

        // If the offset is outside the 0-7 range, the current FIFO x position is not covered by this sprite.
        if (offset < 0 || offset > 7)
            continue;

        // Determine the bit position for fetching color index, potentially flipping it for x-flipped sprites.
        bitPos = 7 - offset;
        if (fetchedSprites.at(i).attributes.xFlip)
            bitPos = offset;

        // Fetch the color index from the sprite's tile data, which is a 2-bit index into the sprite's palette.
        uint8_t colorIdx = (!!(pixelFifo.oamFetchData.at((i * 2) + 1) & (1 << bitPos)) << 1) | // High bit
                           (!!(pixelFifo.oamFetchData.at((i * 2) + 0) & (1 << bitPos)) << 0);  // Low bit

        // If the color index is 0, this sprite pixel is transparent and should be skipped.
        if (!colorIdx)
            continue;

        // Check the sprite's background priority attribute.
        bool bgp = fetchedSprites.at(i).attributes.bgPriority;

        // If the sprite has priority over BG/W or if the BG color is transparent,
        // the sprite pixel color takes precedence and the loop returns the color.
        if (!bgp || bgColorIdx == 0) {
            // Select the appropriate color from the sprite's palette based on the dmgPalette attribute.
            color = fetchedSprites.at(i).attributes.dmgPalette ?
                    lcd->obj1Palette[colorIdx] :
                    lcd->obj0Palette[colorIdx];

            return color;
        }
    }
    return color; // No sprite pixel overrides the BG color, so return the original BG color
}
// =====================================================================================================================

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
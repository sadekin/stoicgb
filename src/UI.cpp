#include "UI.hpp"
#include "GB.hpp"

// Constants defining the dimensions for the main and debug screens.
constexpr int SCALE = 4; // Scaling factor for rendering tiles
constexpr int GB_SCREEN_WIDTH  = 160 * SCALE;
constexpr int GB_SCREEN_HEIGHT = 144 * SCALE;
constexpr int DEBUG_SCREEN_WIDTH  = 16 * 8 * SCALE;
constexpr int DEBUG_SCREEN_HEIGHT = 19 * 8 * SCALE;

/**
 * Constructor for the UI class, initializing SDL and creating the main and debug windows.
 * @param b  Bus (for reading from memory)
 * @param p  PPU (for reading from VRAM)
 * @param gb Game Boy (for accessing the running state)
 * @param j  Joypad (for reading input)
 */
UI::UI(Bus* b, PPU* p, GB* gb, Joypad* j)
: bus(b)
, ppu(p)
, gameBoy(gb)
, joypad(j) {
    // Initialize SDL2 for video and font rendering.
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();

    // Create the main gbWindow and gbRenderer with the specified dimensions.
    SDL_CreateWindowAndRenderer(GB_SCREEN_WIDTH, GB_SCREEN_HEIGHT, 0, &gbWindow, &gbRenderer);

    // Create an SDL surface for the GB window.
    gbScreen = SDL_CreateRGBSurface(
        0,                  // Flags
        GB_SCREEN_WIDTH,    // Width of the surface
        GB_SCREEN_HEIGHT,   // Height of the surface
        32,                 // Bit depth (32 bits for ARGB format)
        0x00FF0000,         // Red component bit mask
        0x0000FF00,         // Green component bit mask
        0x000000FF,         // Blue component bit mask
        0xFF000000          // Alpha component bit mask
    );

    // Create an SDL texture for the GB renderer.
    gbTexture = SDL_CreateTexture(
        gbRenderer,                     // Renderer
        SDL_PIXELFORMAT_ARGB8888,       // Pixel format (32-bit ARGB)
        SDL_TEXTUREACCESS_STREAMING,    // Access pattern (streaming for frequent updates)
        GB_SCREEN_WIDTH,                // Texture width
        GB_SCREEN_HEIGHT                // Texture height
    );


    // Create the debug window and renderer, used for displaying debug information.
    SDL_CreateWindowAndRenderer(DEBUG_SCREEN_WIDTH, DEBUG_SCREEN_HEIGHT, 0, &debugWindow, &debugRenderer);

    // Create an SDL surface for the debug window.
    // The surface is sized to fit all Game Boy tiles (384 tiles in a 24x16 grid) with scaling applied.
    debugScreen = SDL_CreateRGBSurface(
        0,                                  // Flags
        DEBUG_SCREEN_WIDTH  + (16 * SCALE), // Width of the surface
        DEBUG_SCREEN_HEIGHT + (64 * SCALE), // Height of the surface
        32,                                 // Bit depth (32 bits for ARGB format)
        0x00FF0000,                         // Red component bit mask
        0x0000FF00,                         // Green component bit mask
        0x000000FF,                         // Blue component bit mask
        0xFF000000                          // Alpha component bit mask
    );

    // Create an SDL texture for the debug renderer.
    debugTexture = SDL_CreateTexture(
        debugRenderer,                      // Renderer
        SDL_PIXELFORMAT_ARGB8888,           // Pixel format
        SDL_TEXTUREACCESS_STREAMING,        // Access pattern (streaming for frequent updates)
        DEBUG_SCREEN_WIDTH  + (16 * SCALE), // Texture width
        DEBUG_SCREEN_HEIGHT + (64 * SCALE)  // Texture height
    );

    // Align the debug window next to the main gbWindow.
    int windowPosX, windowPosY;
    SDL_GetWindowPosition(gbWindow, &windowPosX, &windowPosY);
    SDL_SetWindowPosition(gbWindow, windowPosX - 150, windowPosY);
    SDL_SetWindowPosition(debugWindow, windowPosX - 140 + GB_SCREEN_WIDTH, windowPosY);
}

/**
 * Destructor for the UI class, cleaning up SDL resources.
 */
UI::~UI() {
    if (gbTexture) SDL_DestroyTexture(gbTexture);
    if (gbRenderer) SDL_DestroyRenderer(gbRenderer);
    if (gbWindow) SDL_DestroyWindow(gbWindow);
    if (debugTexture) SDL_DestroyTexture(debugTexture);
    if (debugRenderer) SDL_DestroyRenderer(debugRenderer);
    if (debugWindow) SDL_DestroyWindow(debugWindow);
    SDL_Quit();
    TTF_Quit();
}

/**
 * SDL2Timing namespace for SDL2 timing functions.
 * These functions are used for timing the main loop and rendering.
 */
namespace SDL2Timing {
    // Waits for the specified number of milliseconds.
    void delay(std::chrono::milliseconds duration) { SDL_Delay(static_cast<uint32_t>(duration.count())); }

    // Returns the number of milliseconds since SDL2 was initialized.
    uint32_t getTicks() { return SDL_GetTicks(); }
}

/**
 * Updates the main gbScreen with the current state of the PPU's video buffer.
 */
void UI::update() {
    // Define a rectangle covering the entire gbScreen.
    SDL_Rect rect;
    rect.x = rect.y = 0;
    rect.w = rect.h = 0;

    // Loop through each pixel in the video buffer and render it on the gbScreen.
    // `ly` is the current scanline being rendered.
    for (int ly = 0; ly < LCD::Y_RESOLUTION; ly++) {
        for (int lx = 0; lx < LCD::X_RESOLUTION; lx++) {
            rect.x = lx * SCALE;      // Horizontal position, accounting for scale
            rect.y = ly * SCALE;      // Vertical position, accounting for scale
            rect.w = rect.h = SCALE;  // Size of the pixel (scaled)

            // Fill the rectangle on the gbScreen with the corresponding color.
            SDL_FillRect(gbScreen, &rect, ppu->videoBuffer[lx + (ly * LCD::X_RESOLUTION)]);
        }
    }

    SDL_UpdateTexture(gbTexture, nullptr, gbScreen->pixels, gbScreen->pitch);
    SDL_RenderClear(gbRenderer);

    // Define the destination rectangle for rendering the texture
    SDL_Rect dstRect;
    dstRect.x = (GB_SCREEN_WIDTH - LCD::X_RESOLUTION * 3.5) / 2;  // Center horizontally
    dstRect.y = (GB_SCREEN_HEIGHT - LCD::Y_RESOLUTION * 3.5) / 2; // Center vertically
    dstRect.w = LCD::X_RESOLUTION * 3.5;                          // Scaled width
    dstRect.h = LCD::Y_RESOLUTION * 3.5;                          // Scaled height

    // Render the texture to the center of the window
    SDL_RenderCopy(gbRenderer, gbTexture, nullptr, &dstRect);

    SDL_RenderPresent(gbRenderer);

    updateDebugWindow();
}

/**
 * Handles SDL events, such as window close events and key presses.
 */
void UI::handleEvents() {
    SDL_Event e;
    while (SDL_PollEvent(&e) > 0) {
        if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_CLOSE) {
            if (gameBoy->cartridge->needsToSave()) {
                gameBoy->cartridge->save();
                std::cout << "Saved cartridge data" << std::endl;
            }
            gameBoy->running = false;
            gameBoy->die = true;
        } else if (e.type == SDL_KEYDOWN) {
            auto key = e.key.keysym.sym;
            if (key == SDLK_ESCAPE) {
                if (gameBoy->cartridge->needsToSave()) {
                    gameBoy->cartridge->save();
                    std::cout << "Saved cartridge data" << std::endl;
                }
                gameBoy->running = false;
                gameBoy->die = true;
            }

            if (key == SDLK_SPACE)  joypad->select = true;
            if (key == SDLK_RETURN) joypad->start = true;
            if (key == SDLK_UP)     joypad->up = true;
            if (key == SDLK_DOWN)   joypad->down = true;
            if (key == SDLK_LEFT)   joypad->left = true;
            if (key == SDLK_RIGHT)  joypad->right = true;
            if (key == SDLK_z)      joypad->a = true;
            if (key == SDLK_x)      joypad->b = true;

        } else if (e.type == SDL_KEYUP) {
            auto key = e.key.keysym.sym;

            if (key == SDLK_SPACE)  joypad->select = false;
            if (key == SDLK_RETURN) joypad->start = false;
            if (key == SDLK_UP)     joypad->up = false;
            if (key == SDLK_DOWN)   joypad->down = false;
            if (key == SDLK_LEFT)   joypad->left = false;
            if (key == SDLK_RIGHT)  joypad->right = false;
            if (key == SDLK_z)      joypad->a = false;
            if (key == SDLK_x)      joypad->b = false;
        }
    }
}

// Palette colors for rendering tiles (white, light gray, dark gray, black).
// These colors are used for grayscale display emulation.
static std::array<uint32_t, 4> tilePalette = { 0xFFFFFFFF, 0xFFAAAAAA, 0xFF555555, 0xFF000000 };

/**
 * Renders a single tile from the Game Boy's VRAM onto the provided SDL_Surface.
 * This function translates the tile data from VRAM into visual pixels on the given surface.
 * See: https://www.huderlem.com/demos/gameboy2bpp.html
 *
 * @param surface SDL surface to render the tile on
 * @param tileIdx Index of the tile to render
 * @param posX Horizontal position of the tile on the surface
 * @param posY Vertical position of the tile on the surface
 */
void UI::displayTile(SDL_Surface* surface, uint16_t currTileNum, int posX, int posY) {
    SDL_Rect tileRect;

    for (int tileY = 0; tileY < 16; tileY += 2) {
        // Read two bytes representing the color data for 8 pixels (2 bits per pixel => 16 bytes).
        // Tile data is stored in VRAM in the memory area at $8000-$97FF.
        uint8_t loByte = ppu->readVRAM(0x8000 + (currTileNum * 16) + (tileY + 0));
        uint8_t hiByte = ppu->readVRAM(0x8000 + (currTileNum * 16) + (tileY + 1));

        // Iterate over each bit in the bytes (from MSB to LSB) to determine the color of each pixel.
        for (int bitPos = 7; bitPos >= 0; bitPos--) {
            // Extract and combine the higher and lower bits of the two-bitPos color code for each pixel.
            // Note: Double negation `!!` normalizes the operand (the bit in this case) to either 0 or 1
            uint8_t colorIdx = !!(loByte & (1 << bitPos)) << 1 | // high bit from loByte
                               !!(hiByte & (1 << bitPos)) << 0;  // low bit from hiByte

            // Calculate the position and size of the pixel on the surface.
            tileRect.x = posX + ((7 - bitPos) * SCALE);
            tileRect.y = posY + (tileY / 2 * SCALE);
            tileRect.w = SCALE;
            tileRect.h = SCALE;

            // Fill the rectangle on the surface with the corresponding color.
            SDL_FillRect(surface, &tileRect, tilePalette[colorIdx]);
        }
    }
}

/**
 * Updates the debug window by rendering the Game Boy's VRAM tiles.
 * This is used for debugging and visualization of the game's tile data.
 */
void UI::updateDebugWindow() {
    // Initialize variables for tracking the position where tiles are drawn on the debug gbScreen.
    int drawPosX = 0, drawPosY = 0;
    int currTileNum = 0;

    // Define a rectangle covering the entire debug gbScreen.
    SDL_Rect rect;
    rect.x = rect.y = 0;
    rect.w = debugScreen->w;
    rect.h = debugScreen->h;
    SDL_FillRect(debugScreen, &rect, 0xFF111111);

    // VRAM can store 384 tiles, arranged in a 24x16 grid for debug visualization.
    for (int y = 0; y < 24; y++) {
        for (int x = 0; x < 16; x++) {
            // Display each tile on the debug gbScreen at the calculated position.
            displayTile(debugScreen, currTileNum, drawPosX + (x * SCALE), drawPosY + (y * SCALE));

            // Move to the next tile position horizontally.
            drawPosX += (8 * SCALE);
            currTileNum++;
        }
        // Move to the next row and reset horizontal position.
        drawPosY += (8 * SCALE);
        drawPosX = 0;
    }

    // Update the gbTexture with the rendered pixel data from the debug gbScreen.
    SDL_UpdateTexture(debugTexture, nullptr, debugScreen->pixels, debugScreen->pitch);

    // Render the updated gbTexture to the debug gbRenderer.
    SDL_RenderClear(debugRenderer);
    SDL_RenderCopy(debugRenderer, debugTexture, nullptr, nullptr);
    SDL_RenderPresent(debugRenderer);
}
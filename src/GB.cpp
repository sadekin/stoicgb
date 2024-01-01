#include "GB.hpp"
#include "../lib/tinyfiledialogs/tinyfiledialogs.hpp"

GB::GB() {
    // Get the ROM file path from the user using a file dialog.
    cartridge = new Cartridge(
            tinyfd_openFileDialog(
                "Choose ROM",           // Dialog title
                "",                     // Default path and file
                0,                      // Number of filter patterns
                nullptr,                // Filter patterns
                nullptr,                // Single filter description
                0                       // Allow multiple selects
            )
    );

    // TODO: Remove this monstrosity
    // Blargg Tests ----------------------------------------------------------------------------------------------------
//    cartridge = new Cartridge("../roms/blargg/dmg-acid2.gb");                                     // OK

    // DMG Sound
//    cartridge = new Cartridge("../roms/blargg/dmg_sound/rom_singles/01-registers.gb");            // Passes
//    cartridge = new Cartridge("../roms/blargg/dmg_sound/rom_singles/02-len ctr.gb");              // Passes
//    cartridge = new Cartridge("../roms/blargg/dmg_sound/rom_singles/03-trigger.gb");              // TODO
//    cartridge = new Cartridge("../roms/blargg/dmg_sound/rom_singles/04-sweep.gb");                // Passes
//    cartridge = new Cartridge("../roms/blargg/dmg_sound/rom_singles/05-sweep details.gb");        // TODO
//    cartridge = new Cartridge("../roms/blargg/dmg_sound/rom_singles/06-overflow on trigger.gb");  // Passes
//    cartridge = new Cartridge("../roms/blargg/dmg_sound/rom_singles/07-len sweep period sync.gb");// TODO
//    cartridge = new Cartridge("../roms/blargg/dmg_sound/rom_singles/08-len ctr during power.gb"); // TODO
//    cartridge = new Cartridge("../roms/blargg/dmg_sound/rom_singles/09-wave read while on.gb");   // TODO
//    cartridge = new Cartridge("../roms/blargg/dmg_sound/rom_singles/10-wave trigger while on.gb");// TODO
//    cartridge = new Cartridge("../roms/blargg/dmg_sound/rom_singles/11-regs after power.gb");     // Passes
//    cartridge = new Cartridge("../roms/blargg/dmg_sound/rom_singles/12-wave write while on.gb");  // TODO

    // Mem Timing
//    cartridge = new Cartridge("../roms/blargg/mem_timing/mem_timing.gb");                         // Passes
//    cartridge = new Cartridge("../roms/blargg/mem_timing-2/mem_timing.gb");                       // Passes

//    cartridge = new Cartridge("../roms/blargg/cpu_instrs/cpu_instrs.gb");                         // Passes

    // Interrupt Timing
//    cartridge = new Cartridge("../roms/blargg/interrupt_time/interrupt_time.gb");                 // TODO: Passes DMG normal speed

    // Instruction Timing
//    cartridge = new Cartridge("../roms/blargg/instr_timing/instr_timing.gb");                     // Passes!!!
    // -----------------------------------------------------------------------------------------------------------------

    // Mooneye Tests ---------------------------------------------------------------------------------------------------
    // Bits
//    cartridge = new Cartridge("../roms/mooneye/bits/mem_oam.gb");                     // OK
//    cartridge = new Cartridge("../roms/mooneye/bits/reg_f.gb");                       // OK
//    cartridge = new Cartridge("../roms/mooneye/bits/unused_hwio-GS.gb");              // OK

    // Instr
//    cartridge = new Cartridge("../roms/mooneye/instr/daa.gb");                        // OK

    // Interrupts
//    cartridge = new Cartridge("../roms/mooneye/interrupts/ie_push.gb");               // OK

    // OAM DMA
//    cartridge = new Cartridge("../roms/mooneye/oam_dma/basic.gb");                    // OK
//    cartridge = new Cartridge("../roms/mooneye/oam_dma/reg_read.gb");                 // OK
//    cartridge = new Cartridge("../roms/mooneye/oam_dma/sources-GS.gb");               // TODO: Fails (OAM DMA stuff)

    // PPU
//    cartridge = new Cartridge("../roms/mooneye/ppu/hblank_ly_scx_timing-GS.gb");      // TODO
//    cartridge = new Cartridge("../roms/mooneye/ppu/intr_1_2_timing-GS.gb");           // TODO
//    cartridge = new Cartridge("../roms/mooneye/ppu/intr_2_0_timing.gb");              // TODO
//    cartridge = new Cartridge("../roms/mooneye/ppu/intr_2_mode0_timing.gb");          // TODO
//    cartridge = new Cartridge("../roms/mooneye/ppu/intr_2_mode0_timing_sprites.gb");  // TODO
//    cartridge = new Cartridge("../roms/mooneye/ppu/intr_2_mode3_timing.gb");          // TODO
//    cartridge = new Cartridge("../roms/mooneye/ppu/intr_2_oam_ok_timing.gb");         // TODO
//    cartridge = new Cartridge("../roms/mooneye/ppu/lcdon_timing-GS.gb");              // TODO
//    cartridge = new Cartridge("../roms/mooneye/ppu/lcdon_write_timing-GS.gb");        // TODO
//    cartridge = new Cartridge("../roms/mooneye/ppu/stat_irq_blocking.gb");            // TODO
//    cartridge = new Cartridge("../roms/mooneye/ppu/stat_lyc_onoff.gb");               // TODO
//    cartridge = new Cartridge("../roms/mooneye/ppu/vblank_stat_intr-GS.gb");          // TODO

    // Serial
//    cartridge = new Cartridge("../roms/mooneye/serial/boot_sclk_align-dmgABCmgb.gb"); // OK

    // Timer
//    cartridge = new Cartridge("../roms/mooneye/timer/div_write.gb");                  // OK
//    cartridge = new Cartridge("../roms/mooneye/timer/tim00.gb");                      // OK
//    cartridge = new Cartridge("../roms/mooneye/timer/tim01.gb");                      // OK
//    cartridge = new Cartridge("../roms/mooneye/timer/tim00_div_trigger.gb");          // OK
//    cartridge = new Cartridge("../roms/mooneye/timer/tim01.gb");                      // OK
//    cartridge = new Cartridge("../roms/mooneye/timer/tim01_div_trigger.gb");          // OK
//    cartridge = new Cartridge("../roms/mooneye/timer/tim10.gb");                      // OK
//    cartridge = new Cartridge("../roms/mooneye/timer/tim10_div_trigger.gb");          // OK
//    cartridge = new Cartridge("../roms/mooneye/timer/tim11.gb");                      // OK
//    cartridge = new Cartridge("../roms/mooneye/timer/tim11_div_trigger.gb");          // OK
//    cartridge = new Cartridge("../roms/mooneye/timer/tima_reload.gb");                // OK
//    cartridge = new Cartridge("../roms/mooneye/timer/tima_write_reloading.gb");       // OK

    // Misc
//    cartridge = new Cartridge("../roms/mooneye/add_sp_e_timing.gb");                  // TODO: Fails (OAM WRAM stuff)
//    cartridge = new Cartridge("../roms/mooneye/boot_div2-S.gb");                      // SGB only
//    cartridge = new Cartridge("../roms/mooneye/boot_div-dmg0.gb");                    // DMG0 only
//    cartridge = new Cartridge("../roms/mooneye/boot_div-dmgABCmgb.gb");               // TODO
//    cartridge = new Cartridge("../roms/mooneye/boot_div-S.gb");                       // SGB only
//    cartridge = new Cartridge("../roms/mooneye/boot_hwio-dmg0.gb");                   // DMG0 only
//    cartridge = new Cartridge("../roms/mooneye/boot_hwio-dmgABCmgb.gb");              // TODO: Passes only when components are manually initialized, not after running boot rom
//    cartridge = new Cartridge("../roms/mooneye/boot_hwio-S.gb");                      // SGB only
//    cartridge = new Cartridge("../roms/mooneye/boot_regs-dmg0.gb");                   // DMG0 only
//    cartridge = new Cartridge("../roms/mooneye/boot_regs-dmgABC.gb");                 // OK
//    cartridge = new Cartridge("../roms/mooneye/boot_regs-mgb.gb");                    // MGB only
//    cartridge = new Cartridge("../roms/mooneye/boot_regs-sgb.gb");                    // SGB only
//    cartridge = new Cartridge("../roms/mooneye/boot_regs-sgb2.gb");                   // SGB2 only
//    cartridge = new Cartridge("../roms/mooneye/call_cc_timing.gb");                   // TODO
//    cartridge = new Cartridge("../roms/mooneye/call_cc_timing2.gb");                  // TODO
//    cartridge = new Cartridge("../roms/mooneye/call_timing.gb");                      // TODO
//    cartridge = new Cartridge("../roms/mooneye/di_timing-GS.gb");                     // OK
//    cartridge = new Cartridge("../roms/mooneye/div_timing.gb");                       // OK
//    cartridge = new Cartridge("../roms/mooneye/ei_sequence.gb");                      // OK
//    cartridge = new Cartridge("../roms/mooneye/ei_timing.gb");                        // OK
//    cartridge = new Cartridge("../roms/mooneye/halt_ime0_ei.gb");                     // OK
//    cartridge = new Cartridge("../roms/mooneye/halt_ime0_nointr_timing.gb");          // OK
//    cartridge = new Cartridge("../roms/mooneye/halt_ime1_timing.gb");                 // OK
//    cartridge = new Cartridge("../roms/mooneye/halt_ime1_timing2-GS.gb");             // OK
//    cartridge = new Cartridge("../roms/mooneye/if_ie_registers.gb");                  // TODO: Fails (I think I need to handle STAT interrupt timing)
//    cartridge = new Cartridge("../roms/mooneye/intr_timing.gb");                      // OK
//    cartridge = new Cartridge("../roms/mooneye/jp_cc_timing.gb");                     // TODO: Fails (OAM DMA stuff)
//    cartridge = new Cartridge("../roms/mooneye/jp_timing.gb");                        // TODO: Fails (OAM DMA stuff)
//    cartridge = new Cartridge("../roms/mooneye/ld_hl_sp_e_timing.gb");                // TODO: Fails (OAM DMA stuff)
//    cartridge = new Cartridge("../roms/mooneye/oam_dma_restart.gb");                  // TODO: Fails (OAM DMA stuff)
//    cartridge = new Cartridge("../roms/mooneye/oam_dma_start.gb");                    // TODO: Fails (OAM DMA stuff)
//    cartridge = new Cartridge("../roms/mooneye/oam_dma_timing.gb");                   // TODO: Fails (OAM DMA stuff)
//    cartridge = new Cartridge("../roms/mooneye/pop_timing.gb");                       // OK
//    cartridge = new Cartridge("../roms/mooneye/push_timing.gb");                      // TODO: Fails (OAM DMA stuff)
//    cartridge = new Cartridge("../roms/mooneye/rapid_di_ei.gb");                      // OK
//    cartridge = new Cartridge("../roms/mooneye/ret_cc_timing.gb");                    // TODO: Fails (OAM DMA stuff)
//    cartridge = new Cartridge("../roms/mooneye/ret_timing.gb");                       // TODO: Fails (OAM DMA stuff)
//    cartridge = new Cartridge("../roms/mooneye/reti_intr_timing.gb");                 // OK
//    cartridge = new Cartridge("../roms/mooneye/reti_timing.gb");                      // TODO: Fails (OAM DMA stuff)
//    cartridge = new Cartridge("../roms/mooneye/rst_timing.gb");                       // TODO: Fails (OAM DMA stuff)


    // -----------------------------------------------------------------------------------------------------------------


    // Connect all components
    intHandler = new InterruptHandler();
    timer = new Timer(intHandler);
    joypad = new Joypad(intHandler);
    serial = new Serial(intHandler, timer);

    lcd = new LCD(nullptr, intHandler);
    ppu = new PPU(cartridge, lcd, intHandler);
    lcd->ConnectPPU(ppu);

    dma = new DMA(nullptr, ppu);
    lcd->ConnectDMA(dma);

    apu = new APU();
    io = new IO(intHandler, timer, dma, lcd, joypad, apu, serial);

    bus = new Bus(ppu, cartridge, io, intHandler, timer, dma);
    dma->ConnectBus(bus);

    cpu = new SM83(bus, intHandler, timer, this);

    ui = new UI(bus, ppu, this, joypad);
}

GB::~GB() = default;

/**
 * Executes the CPU run loop.
 * Manages the flow of the CPU operations, including running the boot ROM and the game ROM.
 * The function uses a loop to continuously step through CPU instructions. It includes logic
 * to handle the transition from the boot ROM to the game ROM. The running state is controlled
 * by the 'running' variable, which can be turned off via UI events.
 */
void GB::cpuRun() {
    running = true;
    ticks = 0;

    // TODO: Use open source boot ROM
//    cartridge->bootROMEnabled = true;
    if (!cartridge->bootROMEnabled) {
        cpu->init();
        lcd->init();
        apu->init();
        serial->init();
    }

    // Run the game ROM.
    while (running) {
        if (!cpu->step())
            printf("CPU STOPPED\n");
    }
}

/**
 * Initiates and manages the emulation process.
 * This function creates a new thread for CPU operations using std::thread, allowing
 * the CPU to run independently of the main program flow. This is crucial for
 * maintaining responsive UI updates and handling events without being blocked by the
 * CPU's processing. The function continuously checks for UI updates and handles user
 * input while the CPU executes in a separate thread.
 */
void GB::emuRun() {
    // Start the CPU thread.
    cpuThread = std::thread(&GB::cpuRun, this);

    // Main loop.
    uint32_t prevFramesRendered = 0;
    while (!die) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        ui->handleEvents();
        // Update the UI if a new frame has been rendered.
        if (prevFramesRendered != ppu->framesRendered)
            ui->update();
        prevFramesRendered = ppu->framesRendered;
    }

    // Stop the CPU thread.
    if (cpuThread.joinable()) // Wait for the CPU thread to finish
        cpuThread.join();     // before exiting the program.
}

/**
* Emulates the passage of machine cycles (M-cycles) and clock cycles (T-cycles) in the Game Boy CPU.
*
* The Game Boy CPU operates at 4.194304 MHz (~4 million cycles per second), with each machine
* cycle (M-cycle) consisting of four clock cycles (T-cycles). This function advances the Game
* Boy's hardware components by the specified number of M-cycles, with each component being updated
* accordingly. Each call to a component's tick method represents the passage of one T-cycle.
*
* @param mCycles The number of M-cycles to emulate. Each M-cycle is four T-cycles.
*/
void GB::emulateCycles(int mCycles) {
    for (int m = 0; m < mCycles; m++) {
        for (int t = 0; t < 4; ++t, ++ticks) {
            timer->tick();
            ppu->tick();
            apu->tick();
            serial->tick();
        }
        dma->tick();
    }
}

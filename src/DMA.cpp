#include "DMA.hpp"
#include "Bus.hpp"

DMA::DMA(Bus* b, PPU* p)
: bus(b)
, ppu(p) {}

DMA::~DMA() = default;

/**
 * Initiates a DMA transfer from a specified start address.
 * The DMA transfer copies 160 bytes from the given start address to the OAM (Object Attribute Memory),
 * commonly used for sprite attribute data. This transfer, taking about 160 machine-cycles,
 * effectively blocks the CPU's access to most memory during its operation.
 * The DMA controller sequentially reads bytes from the source address in memory and writes them to the OAM.
 *
 * @param addr The upper byte of the source address from which attributes data will be copied.
 */
void DMA::startTransfer(uint8_t addr) {
    isActive = true;      // Set flag to indicate that DMA transfer is now active.
    addrLowerByte = 0;    // Initialize lower byte of address to start from beginning.
    addrUpperByte = addr; // Set the upper byte of the source address for the transfer.
}

/**
 * Executes a single tick (cycle) of the DMA transfer.
 * This function progresses the DMA transfer by one byte per call. It sequentially transfers
 * data from the source address to OAM. Once all 160 bytes (from $XX00-$XX9F) are transferred
 * (to $FE00-$FE9F), it deactivates the DMA transfer process. Once started, the DMA transfer
 * copies one byte every 4 T-cycles.
 * See https://hacktix.github.io/GBEDG/dma/ and https://gbdev.io/pandocs/OAM_DMA_Transfer.html
 * TODO: Figure out why I don't need a delay after starting the DMA transfer.
 */
void DMA::tick() {
    // Return if DMA transfer is inactive.
    if (!isActive)
        return;

    // Perform the actual byte transfer from the source address to OAM.
    ppu->writeOAM(
        0xFE00 | addrLowerByte,                          // $FE00-$FE9F (OAM address to write to)
        bus->read((addrUpperByte << 8) | addrLowerByte)  // $XX00-$XX9F (Source address of data to transfer)
    );

    // Increment the address byte and check if transfer is complete.
    isActive = ++addrLowerByte < 0xA0;
}

/**
 * Checks if a DMA transfer is currently active.
 *
 * @return True if DMA transfer is in progress, false otherwise.
 */
bool DMA::isTransferring() const {
    return isActive;
}


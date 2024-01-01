#pragma once

#include "common.hpp"
#include "InterruptHandler.hpp"
#include "Timer.hpp"
#include "Bus.hpp"

#define LOGGING false // Set to true to enable logging (will drastically slow down emulation)

class Bus;      // To avoid circular dependency
class GB;  // To avoid circular dependency

class SM83 {
    friend class GB;

public:
    SM83(Bus* b, InterruptHandler* ih, Timer* t, GB* gb);
    ~SM83();

public:
    bool step();  // Emulates the fetch-decode-execute cycle of the CPU
    void init(); // Resets the CPU to a known state

private:
    // CPU core registers. The Accumulator (A) holds the result of arithmetic and logical operations and data.
    // The general purpose registers (B, C, D, E, H, L) are used as auxiliary registers to the Accumulator,
    // and are also used as register pairs (BC, DE, HL) for addressing memory. The Stack Pointer (SP) is used
    // to address the stack, and the Program Counter (PC) is used to point to the next instruction to be
    // executed or the information (the immediate byte or word) required to execute the current instruction.
    uint8_t  a  = 0x00;   // Accumulator register
    uint8_t  f  = 0x00;   // Flag/status register
    uint8_t  b  = 0x00;   // B register
    uint8_t  c  = 0x00;   // C register
    uint8_t  d  = 0x00;   // D register
    uint8_t  e  = 0x00;   // E register
    uint8_t  h  = 0x00;   // H register
    uint8_t  l  = 0x00;   // L register
    uint16_t sp = 0x0000; // Stack pointer
    uint16_t pc = 0x0000; // Program counter

    // CPU flags. The flag register (F) holds information that is set or cleared as a result of the most
    // recently executed instruction. The flags are used to indicate the results of arithmetic and logical
    // operations, and to enable conditional branching. Note that the lower 4 bits are unused and are always 0.
    enum FlagSM83 {
        C = (1 << 4), // Carry
        H = (1 << 5), // Half-Carry
        N = (1 << 6), // Negative/Subtract
        Z = (1 << 7), // Zero
    };

private:
    uint8_t read(uint16_t addr);                 // Reads a byte from bus
    void    write(uint16_t addr, uint8_t data);  // Writes a byte to bus

private:
    // Assistive variables to facilitate emulation.
    uint8_t  opcode   = 0x00;        // Current instruction byte
    uint16_t fetched  = 0x0000;      // Current fetched data
    bool     dstIsMem = false;       // Indicates if current fetched will be written in memory
    uint16_t memDest  = 0x0000;      // Memory address for when dstIsMem = true
    bool     halted   = false;       // Indicates if CPU is halted (cancelled by an interrupt or reset signal)

    // The Register enum is used for facilitating the reading and writing of registers during the
    // execution of an instruction. They are also used for disassembly. Register 'X' is used to
    // indicate that an instruction does not operate on any register.
    enum class Register  { X, A, F, B, C, D, E, H, L, AF, BC, DE, HL, SP, PC };

    // The Condition enum is used for facilitating the execution of branching instructions (RET, JP, CALL)
    // The 'X' condition is used to indicate that the instruction does not require a branch decision check,
    // and therefore will always branch.
    enum class Condition { X, NC, C, NZ, Z }; // No carry, carry, not zero, zero

    Register dst      = Register::X; // Current destination register for register addressing
    Register src      = Register::X; // Current source register for register addressing
    uint8_t  temp8    = 0x00;        // An 8-bit convenience variable
    uint16_t temp16   = 0x0000;      // A 16-bit convenience variable
    uint32_t temp32   = 0x000000000; // A 32-bit convenience variable
    uint8_t  xx       = 0x00;        // A convenience variable for decoding CB-prefixed instructions
    uint8_t  yyy      = 0x00;        // A convenience variable for decoding CB-prefixed instructions
    uint8_t  zzz      = 0x00;        // A convenience variable for decoding CB-prefixed instructions

    // Since decoding the Game Boy CPU opcodes is a pain, I've decided to use a lookup table
    // for the following structure, which is also useful for disassembly.
    struct Instruction {
        std::string name;                             // Name of opcode for disassembly
        void        (SM83::*operate )() = nullptr;    // Opcode function pointer
        void        (SM83::*addrmode)() = &SM83::IMP; // Addressing mode function pointer (default is implied)
        Register    dstReg;                           // Destination register for register/memory addressing
        Register    srcReg;                           // Source register for register/memory addressing
        Condition   cond = Condition::X;              // For branching instructions (default to 'X'=None=true)
        uint8_t     param;                            // For RST instructions (0x00, 0x08, 0x10, ... , or 0x38)
    };

    std::vector<Instruction> lookup;                  // Instruction lookup table
    std::vector<Instruction> cbLookup;                // CB-prefixed instruction lookup table
    Instruction*             instruction;             // Pointer to current instruction

    void fetch();                                     // Fetches next instruction
    void execute();                                   // Executes current instruction
    void emulateCycles(int mCycles) const;            // Emulates the execution of machine (M) cycles
    void handleInterrupts();                          // Handles interrupts

private:
    // Addressing modes ===========================================================
    // The various addressing modes of the Game Boy's CPU essentially ensure that
    // the program counter is at the correct location in memory at the right time
    // to facilitate execution of an instruction. Each opcode has an addressing
    // mode, a function which is called prior to execution to fetch (or not to
    // fetch in the case of the "implied" mode) the information required to
    // facilitate the execution of the instruction.
    //
    // These functions can modify the number of machine cycles required to execute
    // an instruction depending on whether they need to fetch data from memory.
    // They also indicate to certain instructions whether data needs to be
    // written to memory or not.
    //
    // Some modes might appear redundant, but, since decoding the Game Boy CPU's
    // opcodes is not as straightforward as, say, the Intel 8080's opcodes, they
    // seemed necessary to keep the code clean(er) and (more) readable. They were
    // not only useful for disassembly, but they also made it possible to not have
    // to implement all the 512 opcodes individually in some switch statement.
    // A description of each mode is above the respective implementation.

    void IMP();      void   R_R();    void MR_D8();    void  R_HLI();
    void   R();      void  R_D8();    void  R_A8();    void  R_HLD();
    void  MR();      void  R_MR();    void  A8_R();    void  HLI_R();
    void  D8();      void  MR_R();    void R_A16();    void  HLD_R();
    void D16();      void R_D16();    void A16_R();    void HL_SPR();

private:
    // Opcodes =====================================================================
    // Including the CB-prefixed ones, these cover all 512 of them. A short
    // description of each opcode along with the timing information can be found
    // above the respective implementation. The XXX opcode is used to capture
    // all unimplemented instructions.

    void  ADC();     void  CPL();     void   JP();     void  RET();     void  SBC();
    void  ADD();     void  DAA();     void   JR();     void RETI();     void  SCF();
    void  AND();     void  DEC();     void   LD();     void RLCA();     void STOP();
    void CALL();     void   DI();     void  LDH();     void  RLA();     void  SUB();
    void   CB();     void   EI();     void  NOP();     void  RRA();     void   OR();
    void  CCF();     void HALT();     void  POP();     void RRCA();     void  XOR();
    void   CP();     void  INC();     void PUSH();     void  RST();     void  XXX();

    // CB-Prefixed Opcodes =========================================================
    // These opcodes all take a minimum of 2 machine cycles to execute: One byte is
    // fetched to determine that the opcode is a CB-prefixed one, and another byte
    // is fetched to determine which CB-prefixed opcode to execute. They perform
    // bitwise operations on the Accumulator as well as on the general-purpose
    // registers and memory location (HL).

    void BIT();      void  SET();
    void RES();      void  SLA();
    void  RL();      void  SRA();
    void RLC();      void  SRL();
    void  RR();      void SWAP();
    void RRC();


private:
    // Register helper methods
    uint16_t readReg(Register r);
    void     writeReg(Register r, uint16_t data);
    bool     is16Bit(Register r);

    // Flag helper methods
    void    setFlag(FlagSM83 flag, bool v);
    uint8_t getFlag(FlagSM83 flag) const;
    bool    testCond(Condition cond);
    bool    carry(uint32_t x, uint32_t y, uint32_t result, uint32_t mask);
    bool    borrow(uint32_t x, uint32_t y, uint32_t result, uint32_t mask);

private:
    Bus*              bus;
    Timer*            timer;
    InterruptHandler* intHandler;
    GB*          gameBoy;

#if LOGGING
private: // For testing/disassembly
    uint16_t    logPC    = 0x0000;
    uint64_t    logTicks = 0x0000000000000000;
    std::string registerToString(Register r);
    std::string conditionToString(Condition cc);
    void        disassemble();
#endif
};

#include "SM83.hpp"
#include "Bus.hpp"
#include "GB.hpp"


SM83::SM83(Bus* b, InterruptHandler* ih, Timer* t, GB* gb)
: bus(b)
, intHandler(ih)
, timer(t)
, gameBoy(gb)
, instruction(nullptr) {
    using a = SM83;
    using r = Register;
    using c = Condition;
    lookup = {                       /* x0 */                                        /* x1 */                                  /* x2 */                                          /* x3 */                                     /* x4 */                                           /* x5 */                                  /* x6 */                                          /* x7 */                                             /* x8 */                                        /* x9 */                                   /* xA */                                      /* xB */                                    /* xC */                                       /* xD */                                /* xE */                                        /* xF */
    /* 0x */    { "NOP" , &a::NOP                               },{ "LD" , &a::LD , &a::R_D16, r::BC       },{ "LD" , &a::LD , &a::MR_R , r::BC, r::A        },{ "INC", &a::INC, &a::R   , r::BC       },{ "INC" , &a::INC , &a::R   , r::B               },{ "DEC" , &a::DEC , &a::R   , r::B         },{ "LD"  , &a::LD , &a::R_D8 , r::B        },{ "RLCA", &a::RLCA                                   },{ "LD" , &a::LD,  &a::A16_R , r::X , r::SP       },{ "ADD" , &a::ADD, &a::R_R, r::HL, r::BC },{ "LD" , &a::LD , &a::R_MR , r::A, r::BC      },{ "DEC", &a::DEC, &a::R  , r::BC       },{ "INC" , &a::INC , &a::R  , r::C             },{ "DEC" , &a::DEC , &a::R  , r::C       },{ "LD" , &a::LD , &a::R_D8, r::C        },{ "RRCA", &a::RRCA                                  }, // 0x
    /* 1x */    { "STOP", &a::STOP                              },{ "LD" , &a::LD , &a::R_D16, r::DE       },{ "LD" , &a::LD , &a::MR_R , r::DE, r::A        },{ "INC", &a::INC, &a::R   , r::DE       },{ "INC" , &a::INC , &a::R   , r::D               },{ "DEC" , &a::DEC , &a::R   , r::D         },{ "LD"  , &a::LD , &a::R_D8 , r::D        },{ "RLA" , &a::RLA                                    },{ "JR" , &a::JR,  &a::D8                         },{ "ADD" , &a::ADD, &a::R_R, r::HL, r::DE },{ "LD" , &a::LD , &a::R_MR , r::A, r::DE      },{ "DEC", &a::DEC, &a::R  , r::DE       },{ "INC" , &a::INC , &a::R  , r::E             },{ "DEC" , &a::DEC , &a::R  , r::E       },{ "LD" , &a::LD , &a::R_D8, r::E        },{ "RRA" , &a::RRA                                   }, // 1x
    /* 2x */    { "JR"  , &a::JR , &a::D8  , r::X , r::X, c::NZ },{ "LD" , &a::LD , &a::R_D16, r::HL       },{ "LD" , &a::LD , &a::HLI_R, r::HL, r::A        },{ "INC", &a::INC, &a::R   , r::HL       },{ "INC" , &a::INC , &a::R   , r::H               },{ "DEC" , &a::DEC , &a::R   , r::H         },{ "LD"  , &a::LD , &a::R_D8 , r::H        },{ "DAA" , &a::DAA                                    },{ "JR" , &a::JR,  &a::D8    , r::X , r::X , c::Z },{ "ADD" , &a::ADD, &a::R_R, r::HL, r::HL },{ "LD" , &a::LD , &a::R_HLI, r::A, r::HL      },{ "DEC", &a::DEC, &a::R  , r::HL       },{ "INC" , &a::INC , &a::R  , r::L             },{ "DEC" , &a::DEC , &a::R  , r::L       },{ "LD" , &a::LD , &a::R_D8, r::L        },{ "CPL" , &a::CPL                                   }, // 2x
    /* 3x */    { "JR"  , &a::JR , &a::D8  , r::X , r::X, c::NC },{ "LD" , &a::LD , &a::R_D16, r::SP       },{ "LD" , &a::LD , &a::HLD_R, r::HL, r::A        },{ "INC", &a::INC, &a::R   , r::SP       },{ "INC" , &a::INC , &a::MR  , r::HL              },{ "DEC" , &a::DEC , &a::MR  , r::HL        },{ "LD"  , &a::LD , &a::MR_D8, r::HL       },{ "SCF" , &a::SCF                                    },{ "JR" , &a::JR,  &a::D8    , r::X , r::X , c::C },{ "ADD" , &a::ADD, &a::R_R, r::HL, r::SP },{ "LD" , &a::LD , &a::R_HLD, r::A, r::HL      },{ "DEC", &a::DEC, &a::R  , r::SP       },{ "INC" , &a::INC , &a::R  , r::A             },{ "DEC" , &a::DEC , &a::R  , r::A       },{ "LD" , &a::LD , &a::R_D8, r::A        },{ "CCF" , &a::CCF                                   }, // 3x
    /* 4x */    { "LD"  , &a::LD , &a::R_R , r::B , r::B        },{ "LD" , &a::LD , &a::R_R  , r::B , r::C },{ "LD" , &a::LD , &a::R_R  , r::B , r::D        },{ "LD" , &a::LD , &a::R_R , r::B,  r::E },{ "LD"  , &a::LD  , &a::R_R , r::B , r::H        },{ "LD"  , &a::LD  , &a::R_R , r::B ,  r::L },{ "LD"  , &a::LD , &a::R_MR , r::B, r::HL },{ "LD"  , &a::LD , &a::R_R , r::B , r::A             },{ "LD" , &a::LD,  &a::R_R   , r::C , r::B        },{ "LD"  , &a::LD , &a::R_R, r::C,  r::C  },{ "LD" , &a::LD , &a::R_R  , r::C, r::D       },{ "LD" , &a::LD , &a::R_R, r::C,  r::E },{ "LD"  , &a::LD  , &a::R_R, r::C, r::H       },{ "LD"  , &a::LD  , &a::R_R, r::C, r::L },{ "LD" , &a::LD , &a::R_MR, r::C, r::HL },{ "LD"  , &a::LD , &a::R_R, r::C,  r::A             }, // 4x
    /* 5x */    { "LD"  , &a::LD , &a::R_R , r::D , r::B        },{ "LD" , &a::LD , &a::R_R  , r::D , r::C },{ "LD" , &a::LD , &a::R_R  , r::D , r::D        },{ "LD" , &a::LD , &a::R_R , r::D,  r::E },{ "LD"  , &a::LD  , &a::R_R , r::D , r::H        },{ "LD"  , &a::LD  , &a::R_R , r::D ,  r::L },{ "LD"  , &a::LD , &a::R_MR , r::D, r::HL },{ "LD"  , &a::LD , &a::R_R , r::D , r::A             },{ "LD" , &a::LD,  &a::R_R   , r::E , r::B        },{ "LD"  , &a::LD , &a::R_R, r::E,  r::C  },{ "LD" , &a::LD , &a::R_R  , r::E, r::D       },{ "LD" , &a::LD , &a::R_R, r::E,  r::E },{ "LD"  , &a::LD  , &a::R_R, r::E, r::H       },{ "LD"  , &a::LD  , &a::R_R, r::E, r::L },{ "LD" , &a::LD , &a::R_MR, r::E, r::HL },{ "LD"  , &a::LD , &a::R_R, r::E,  r::A             }, // 5x
    /* 6x */    { "LD"  , &a::LD , &a::R_R , r::H , r::B        },{ "LD" , &a::LD , &a::R_R  , r::H , r::C },{ "LD" , &a::LD , &a::R_R  , r::H , r::D        },{ "LD" , &a::LD , &a::R_R , r::H,  r::E },{ "LD"  , &a::LD  , &a::R_R , r::H , r::H        },{ "LD"  , &a::LD  , &a::R_R , r::H ,  r::L },{ "LD"  , &a::LD , &a::R_MR , r::H, r::HL },{ "LD"  , &a::LD , &a::R_R , r::H , r::A             },{ "LD" , &a::LD,  &a::R_R   , r::L , r::B        },{ "LD"  , &a::LD , &a::R_R, r::L,  r::C  },{ "LD" , &a::LD , &a::R_R  , r::L, r::D       },{ "LD" , &a::LD , &a::R_R, r::L,  r::E },{ "LD"  , &a::LD  , &a::R_R, r::L, r::H       },{ "LD"  , &a::LD  , &a::R_R, r::L, r::L },{ "LD" , &a::LD , &a::R_MR, r::L, r::HL },{ "LD"  , &a::LD , &a::R_R, r::L,  r::A             }, // 6x
    /* 7x */    { "LD"  , &a::LD , &a::MR_R, r::HL, r::B        },{ "LD" , &a::LD , &a::MR_R , r::HL, r::C },{ "LD" , &a::LD , &a::MR_R , r::HL, r::D        },{ "LD" , &a::LD , &a::MR_R, r::HL, r::E },{ "LD"  , &a::LD  , &a::MR_R, r::HL, r::H        },{ "LD"  , &a::LD  , &a::MR_R, r::HL,  r::L },{ "HALT", &a::HALT                        },{ "LD"  , &a::LD , &a::MR_R, r::HL, r::A             },{ "LD" , &a::LD,  &a::R_R   , r::A , r::B        },{ "LD"  , &a::LD , &a::R_R, r::A,  r::C  },{ "LD" , &a::LD , &a::R_R  , r::A, r::D       },{ "LD" , &a::LD , &a::R_R, r::A,  r::E },{ "LD"  , &a::LD  , &a::R_R, r::A, r::H       },{ "LD"  , &a::LD  , &a::R_R, r::A, r::L },{ "LD" , &a::LD , &a::R_MR, r::A, r::HL },{ "LD"  , &a::LD , &a::R_R, r::A,  r::A             }, // 7x
    /* 8x */    { "ADD" , &a::ADD, &a::R_R , r::A , r::B        },{ "ADD", &a::ADD, &a::R_R  , r::A , r::C },{ "ADD", &a::ADD, &a::R_R  , r::A , r::D        },{ "ADD", &a::ADD, &a::R_R , r::A,  r::E },{ "ADD" , &a::ADD , &a::R_R , r::A , r::H        },{ "ADD" , &a::ADD , &a::R_R , r::A ,  r::L },{ "ADD" , &a::ADD, &a::R_MR , r::A, r::HL },{ "ADD" , &a::ADD, &a::R_R , r::A , r::A             },{ "ADC", &a::ADC, &a::R_R   , r::A , r::B        },{ "ADC" , &a::ADC, &a::R_R, r::A,  r::C  },{ "ADC", &a::ADC, &a::R_R  , r::A, r::D       },{ "ADC", &a::ADC, &a::R_R, r::A,  r::E },{ "ADC" , &a::ADC , &a::R_R, r::A, r::H       },{ "ADC" , &a::ADC , &a::R_R, r::A, r::L },{ "ADC", &a::ADC, &a::R_MR, r::A, r::HL },{ "ADC" , &a::ADC, &a::R_R, r::A,  r::A             }, // 8x
    /* 9x */    { "SUB" , &a::SUB, &a::R_R , r::A , r::B        },{ "SUB", &a::SUB, &a::R_R  , r::A , r::C },{ "SUB", &a::SUB, &a::R_R  , r::A , r::D        },{ "SUB", &a::SUB, &a::R_R , r::A,  r::E },{ "SUB" , &a::SUB , &a::R_R , r::A , r::H        },{ "SUB" , &a::SUB , &a::R_R , r::A ,  r::L },{ "SUB" , &a::SUB, &a::R_MR , r::A, r::HL },{ "SUB" , &a::SUB, &a::R_R , r::A , r::A             },{ "SBC", &a::SBC, &a::R_R   , r::A , r::B        },{ "SBC" , &a::SBC, &a::R_R, r::A,  r::C  },{ "SBC", &a::SBC, &a::R_R  , r::A, r::D       },{ "SBC", &a::SBC, &a::R_R, r::A,  r::E },{ "SBC" , &a::SBC , &a::R_R, r::A, r::H       },{ "SBC" , &a::SBC , &a::R_R, r::A, r::L },{ "SBC", &a::SBC, &a::R_MR, r::A, r::HL },{ "SBC" , &a::SBC, &a::R_R, r::A,  r::A             }, // 9x
    /* Ax */    { "AND" , &a::AND, &a::R_R , r::A , r::B        },{ "AND", &a::AND, &a::R_R  , r::A , r::C },{ "AND", &a::AND, &a::R_R  , r::A , r::D        },{ "AND", &a::AND, &a::R_R , r::A,  r::E },{ "AND" , &a::AND , &a::R_R , r::A , r::H        },{ "AND" , &a::AND , &a::R_R , r::A ,  r::L },{ "AND" , &a::AND, &a::R_MR , r::A, r::HL },{ "AND" , &a::AND, &a::R_R , r::A , r::A             },{ "XOR", &a::XOR, &a::R_R   , r::A , r::B        },{ "XOR" , &a::XOR, &a::R_R, r::A,  r::C  },{ "XOR", &a::XOR, &a::R_R  , r::A, r::D       },{ "XOR", &a::XOR, &a::R_R, r::A,  r::E },{ "XOR" , &a::XOR , &a::R_R, r::A, r::H       },{ "XOR" , &a::XOR , &a::R_R, r::A, r::L },{ "XOR", &a::XOR, &a::R_MR, r::A, r::HL },{ "XOR" , &a::XOR, &a::R_R, r::A,  r::A             }, // Ax
    /* Bx */    { "OR"  , &a::OR , &a::R_R , r::A , r::B        },{ "OR" , &a::OR , &a::R_R  , r::A , r::C },{ "OR" , &a::OR , &a::R_R  , r::A , r::D        },{ "OR" , &a::OR , &a::R_R , r::A,  r::E },{ "OR"  , &a::OR  , &a::R_R , r::A , r::H        },{ "OR"  , &a::OR  , &a::R_R , r::A ,  r::L },{ "OR"  , &a::OR , &a::R_MR , r::A, r::HL },{ "OR"  , &a::OR , &a::R_R , r::A , r::A             },{ "CP" , &a::CP,  &a::R_R   , r::A , r::B        },{ "CP"  , &a::CP , &a::R_R, r::A,  r::C  },{ "CP" , &a::CP , &a::R_R  , r::A, r::D       },{ "CP" , &a::CP , &a::R_R, r::A,  r::E },{ "CP"  , &a::CP  , &a::R_R, r::A, r::H       },{ "CP"  , &a::CP  , &a::R_R, r::A, r::L },{ "CP" , &a::CP , &a::R_MR, r::A, r::HL },{ "CP"  , &a::CP , &a::R_R, r::A,  r::A             }, // Bx
    /* Cx */    { "RET" , &a::RET, &a::IMP , r::X , r::X, c::NZ },{ "POP", &a::POP, &a::R    , r::BC       },{ "JP" , &a::JP , &a::D16  , r::X , r::X, c::NZ },{ "JP" , &a::JP , &a::D16               },{ "CALL", &a::CALL, &a::D16 , r::X , r::X, c::NZ },{ "PUSH", &a::PUSH, &a::R   , r::BC        },{ "ADD" , &a::ADD, &a::R_D8 , r::A        },{ "RST" , &a::RST, &a::IMP , r::X , r::X, c::X, 0x00 },{ "RET", &a::RET, &a::IMP   , r::X , r::X , c::Z },{ "RET" , &a::RET                        },{ "JP" , &a::JP , &a::D16  , r::X, r::X, c::Z },{ "CB" , &a::CB , &a::D8               },{ "CALL", &a::CALL, &a::D16, r::X, r::X, c::Z },{ "CALL", &a::CALL, &a::D16             },{ "ADC", &a::ADC, &a::R_D8, r::A,       },{ "RST" , &a::RST, &a::IMP, r::X,  r::X, c::X, 0x08 }, // Cx
    /* Dx */    { "RET" , &a::RET, &a::IMP , r::X , r::X, c::NC },{ "POP", &a::POP, &a::R    , r::DE       },{ "JP" , &a::JP , &a::D16  , r::X , r::X, c::NC },{ "XXX", &a::XXX                        },{ "CALL", &a::CALL, &a::D16 , r::X , r::X, c::NC },{ "PUSH", &a::PUSH, &a::R   , r::DE        },{ "SUB" , &a::SUB, &a::R_D8 , r::A        },{ "RST" , &a::RST, &a::IMP , r::X , r::X, c::X, 0x10 },{ "RET", &a::RET, &a::IMP   , r::X , r::X , c::C },{ "RETI", &a::RETI                       },{ "JP" , &a::JP , &a::D16  , r::X, r::X, c::C },{ "XXX", &a::XXX,                      },{ "CALL", &a::CALL, &a::D16, r::X, r::X, c::C },{ "XXX" , &a::XXX ,                     },{ "SBC", &a::SBC, &a::R_D8, r::A,       },{ "RST" , &a::RST, &a::IMP, r::X,  r::X, c::X, 0x18 }, // Dx
    /* Ex */    { "LDH" , &a::LDH, &a::A8_R, r::X , r::A,       },{ "POP", &a::POP, &a::R    , r::HL       },{ "LD" , &a::LD , &a::MR_R , r::C , r::A        },{ "XXX", &a::XXX                        },{ "XXX" , &a::XXX                                },{ "PUSH", &a::PUSH, &a::R   , r::HL        },{ "AND" , &a::AND, &a::R_D8 , r::A        },{ "RST" , &a::RST, &a::IMP , r::X , r::X, c::X, 0x20 },{ "ADD", &a::ADD, &a::R_D8  , r::SP              },{ "JP"  , &a::JP , &a::R , r::HL         },{ "LD" , &a::LD , &a::A16_R, r::X, r::A,      },{ "XXX", &a::XXX,                      },{ "XXX" , &a::XXX ,                           },{ "XXX" , &a::XXX ,                     },{ "XOR", &a::XOR, &a::R_D8, r::A,       },{ "RST" , &a::RST, &a::IMP, r::X,  r::X, c::X, 0x28 }, // Ex
    /* Fx */    { "LDH" , &a::LDH, &a::R_A8, r::A               },{ "POP", &a::POP, &a::R    , r::AF       },{ "LD" , &a::LD , &a::R_MR , r::A , r::C        },{ "DI" , &a::DI                         },{ "XXX" , &a::XXX                                },{ "PUSH", &a::PUSH, &a::R   , r::AF        },{ "OR"  , &a::OR , &a::R_D8 , r::A        },{ "RST" , &a::RST, &a::IMP , r::X , r::X, c::X, 0x30 },{ "LD" , &a::LD , &a::HL_SPR, r::HL, r::SP       },{ "LD"  , &a::LD , &a::R_R, r::SP, r::HL },{ "LD" , &a::LD , &a::R_A16, r::A,            },{ "EI" , &a::EI ,                      },{ "XXX" , &a::XXX ,                           },{ "XXX" , &a::XXX ,                     },{ "CP" , &a::CP , &a::R_D8, r::A,       },{ "RST" , &a::RST, &a::IMP, r::X,  r::X, c::X, 0x38 }, // Fx
    };                              /* x0 */                                        /* x1 */                                  /* x2 */                                          /* x3 */                                     /* x4 */                                           /* x5 */                                  /* x6 */                                          /* x7 */                                             /* x8 */                                        /* x9 */                                   /* xA */                                      /* xB */                                    /* xC */                                       /* xD */                                /* xE */                                        /* xF */

    // Order is important here for decoding CB-prefixed instructions:
    // See table "rot" at https://gb-archive.github.io/salvage/decoding_gbz80_opcodes/Decoding%20Gamboy%20Z80%20Opcodes.html
    // Note: BIT, RES, and SET are called in a switch statement in the SM83::CB function.
    cbLookup = {
            { "RLC" , &a::RLC  }, // 0
            { "RRC" , &a::RRC  }, // 1
            { "RL"  , &a::RL   }, // 2
            { "RR"  , &a::RR   }, // 3
            { "SLA" , &a::SLA  }, // 4
            { "SRA" , &a::SRA  }, // 5
            { "SWAP", &a::SWAP }, // 6
            { "SRL" , &a::SRL  }  // 7
    };
}

SM83::~SM83() = default;

/**
 * Manually sets the CPU to its initial state after running the boot ROM.
 * See: https://gbdev.io/pandocs/Power_Up_Sequence.html#cpu-registers and
 *      https://gbdev.io/pandocs/Power_Up_Sequence.html#hardware-registers
 */
void SM83::init() {
    a = 0x01;
    f = 0xB0;
    b = 0x00;
    c = 0x13;
    d = 0x00;
    e = 0xD8;
    h = 0x01;
    l = 0x4D;

    pc = 0x0100;
    sp = 0xFFFE;

    intHandler->IE = 0x00;
    intHandler->IF = 0xE1;
    intHandler->IME = false;
    intHandler->scheduledIME = false;

    timer->sysClock = 0xABCC;
}

/**
 * Reads an 8-bit value from memory.
 * @param addr Address to read from memory.
 * @return 8-bit value read from memory
 */
uint8_t SM83::read(uint16_t addr) {
    emulateCycles(1);
    return bus->read(addr);
}

/**
 * Writes an 8-bit value to memory.
 * @param addr Address to write to
 * @param data 8-bit value to write
 */
void SM83::write(uint16_t addr, uint8_t data) {
    emulateCycles(1);
    bus->write(addr, data);
}

/**
 * Steps through one instruction. Fetches and executes.
 *
 * @return True if instruction was executed successfully, false otherwise.
 */
bool SM83::step() {
    if (!halted) {
#if LOGGING
        logPC = pc;
        logTicks = gameBoy->ticks;
//        logSB = bus->read(0xFF01);
//        logSC = bus->read(0xFF02);
#endif
        fetch();
        execute();
#if LOGGING
        disassemble();
#endif
    } else { // halted
        emulateCycles(1);
        if (intHandler->interruptRequested())  // IF & IE & 0x1F != 0
            halted = false;
    }

    // Handle interrupts if jumping to interrupt vector is enabled.
    if (intHandler->IME) {
        handleInterrupts();
        intHandler->scheduledIME = false;
    }

    // Enable jumping to interrupt vector if scheduled by EI instruction.
    if (intHandler->scheduledIME) {
        intHandler->IME = true;
        intHandler->scheduledIME = false;
    }

    return true;
}

/**
 * Fetches an instruction from lookup table based on current opcode.
 */
void SM83::fetch() {
    opcode = read(pc++);
    instruction = &lookup[opcode];
}

/**
 * Executes an instruction based on its addressing mode and operation.
 */
void SM83::execute() {
    if (!instruction) {
        std::cerr << "ERROR. Instruction is null. Exiting..." << std::endl;
        std::exit(-1);
    }
    dstIsMem = false;                 // Reset
    (this->*instruction->addrmode)(); // Fetch operands if necessary
    (this->*instruction->operate)();  // Execute instruction
}

/**
 * Handles interrupts based on priority.
 * If an interrupt is requested, the PC is pushed onto the stack and the ISR is called.
 *
 * "It takes 20 clocks to dispatch an interrupt. If CPU is in HALT mode, another extra 4 clocks are needed." -TCAGBD
 */
void SM83::handleInterrupts() {
    if (intHandler->interruptRequested()) { // Note that we do not modify IF here when checking if an interrupt is requested
        // According to TCAGBD, 2 wait states (2 machine cycles or NOPs) pass while nothing occurs.
        // This appears to be correct for passing Blargg's interrupt timing test in normal speed mode.
        emulateCycles(1);
        emulateCycles(1);
        write(--sp, pc >> 8);
        // As I tried to pass the Mooneye ie_push interrupts test, I discovered that it's possible for IE to be
        // written to during the cycle where the upper byte of the PC is pushed onto the stack. This means it's
        // possible for an interrupt dispatch to be cancelled during the dispatching process. The PC is set to
        // $0000 if this happens and IF is not modified.
        intHandler->isr();           // Note that we may modify IF here
        write(--sp, pc & 0xFF);
        intHandler->IME = false;     // Disable jumps to interrupt vectors
        pc = intHandler->isrAddress; // Jump to interrupt vector or $0000 if dispatch cancelled
        emulateCycles(1);            // For modifying PC
        if (halted)
            emulateCycles(1);        // Takes an extra 4 clocks to exit HALT
        halted = false;
    }
}

/**
 * Emulates a number of CPU cycles.
 * This is called after every read and after every write to memory.
 * It's also called to some emulate internal delays.
 *
 * @param mCycles Number of CPU cycles to emulate
 */
void SM83::emulateCycles(int mCycles) const {
    gameBoy->emulateCycles(mCycles);
}

/**
 * Reads CPU registers (8- or 16-bit).
 * @param r Register to read
 * @return Value of register or register pair
 */
uint16_t SM83::readReg(Register r) {
    switch (r) {
        case Register::A:  return a;
        case Register::F:  return f;
        case Register::B:  return b;
        case Register::C:  return c;
        case Register::D:  return d;
        case Register::E:  return e;
        case Register::H:  return h;
        case Register::L:  return l;
        case Register::AF: return ((uint16_t) a << 8) | (uint16_t) f;
        case Register::BC: return ((uint16_t) b << 8) | (uint16_t) c;
        case Register::DE: return ((uint16_t) d << 8) | (uint16_t) e;
        case Register::HL: return ((uint16_t) h << 8) | (uint16_t) l;
        case Register::PC: return pc;
        case Register::SP: return sp;
        default:           return 0;
    }
}

/**
 * Writes CPU registers (8- or 16-bit).
 *
 * @param r Register to write to
 * @param data Data to write to register
 */
void SM83::writeReg(Register r, uint16_t data) {
    switch (r) {
        case Register::A:  a = data & 0xFF; return;
        case Register::F:  f = data & 0xFF; return;
        case Register::B:  b = data & 0xFF; return;
        case Register::C:  c = data & 0xFF; return;
        case Register::D:  d = data & 0xFF; return;
        case Register::E:  e = data & 0xFF; return;
        case Register::H:  h = data & 0xFF; return;
        case Register::L:  l = data & 0xFF; return;
        case Register::AF: a = (data >> 8) & 0xFF; f = data & 0xFF; return;
        case Register::BC: b = (data >> 8) & 0xFF; c = data & 0xFF; return;
        case Register::DE: d = (data >> 8) & 0xFF; e = data & 0xFF; return;
        case Register::HL: h = (data >> 8) & 0xFF; l = data & 0xFF; return;
        case Register::PC: pc = data; return;
        case Register::SP: sp = data; return;
        default:                      return;
    }
}

/**
 * @return True if register is 16-bit, false otherwise.
 */
bool SM83::is16Bit(SM83::Register r) { return static_cast<uint8_t>(r) >= static_cast<uint8_t>(Register::AF); }

/**
 * Sets/clears a flag (Z, N, H, C).
 * @param flag Flag to set/clear.
 * @param v Value to set flag to (0 or 1).
 */
void SM83::setFlag(FlagSM83 flag, bool v) {
    if (v)
        f |= flag;
    else
        f &= ~flag;
}

/**
 * @param flag Flag to get (Z, N, H, C)
 * @return Value of flag (0 or 1)
 */
uint8_t SM83::getFlag(FlagSM83 flag) const {
    return (f & flag) ? 1 : 0;
}

/**
 * Tests a condition.
 *
 * @param cond Condition to test
 * @return True if condition is true, false otherwise.
 */
bool SM83::testCond(Condition cond) {
    switch (cond) {
        case Condition::X : return true;
        case Condition::NZ: return !getFlag(Z);
        case Condition::Z : return getFlag(Z);
        case Condition::NC: return !getFlag(C);
        case Condition::C : return getFlag(C);
        default:            return false;
    }
}

/**
 * Checks if an addition operation resulted in a carry.
 * Basically, the flag is testing the result of upper bits:
 * Carry can occur in one of three cases:
 * 1. msb(a) and msb(b) are set (0.1xxx + 0.1xxx -> 1.0xxx)
 * 2. msb(a) is set but msb(result) is not set (0.1xxx + 0.xxxx -> 1.0xxx)
 * 3. msb(b) is set but msb(result) is not set (0.xxxx + 0.1xxx -> 1.0xxx)
 * Source: https://www.reddit.com/r/EmuDev/comments/110epqk/comment/j89y04a/?utm_source=share&utm_medium=web2x&context=3
 *
 * TODO: Since this doesn't appear to cover all cases, I'll probably remove this.
 *
 * @param a operand 1
 * @param b operand 2
 * @param result result of a + b
 * @param mask mask to apply to result
 * @return
 */
bool SM83::carry(uint32_t a, uint32_t b, uint32_t result, uint32_t mask) {
    return ((a & b) | (a & ~result) | (b & ~result)) & mask;
}

// Checks if a subtraction operation resulted in a borrow.
// a - b = result => a = b + result
bool SM83::borrow(uint32_t a, uint32_t b, uint32_t result, uint32_t mask) {
    return carry(result, b, a, mask);
}


// Addressing modes ====================================================================================================
// Some notation adapted from Pastraiser (https://www.pastraiser.com/cpu/gameboy/gameboy_opcodes.html):
// u8  means immediate unsigned 8-bit data (Pastraiser uses d8)
// u16 means immediate unsigned 16-bit data (Pastraiser uses d16)
// a8  means 8-bit unsigned data, which are added to $FF00 in certain instructions
//     (replacement for missing IN and OUT instructions)
// a16 means 16-bit address
// s8  means 8-bit signed data, which are added to program counter (Pastraiser uses r8)

/**
 * Implied addressing mode is the default addressing mode for instructions that do not have any operands.
 * Ex: NOP, HALT, STOP, DI, EI, ...
 */
void SM83::IMP() {
    // Nothing to do
}

/**
 * Register addressing mode is used for instructions that have a single register operand.
 * Ex: DEC A, INC HL, ...
 */
void SM83::R() {
    fetched = readReg(instruction->dstReg);
}

/**
 * Register to register addressing mode is used for instructions that have two register operands.
 * Ex: LD A,B, LD HL,SP, ...
 */
void SM83::R_R() {
    fetched = readReg(instruction->srcReg);
}

/**
 * Immediate byte addressing mode is used for instructions that have an immediate byte.
 * Ex: JR s8 (signed is accounted for in JR's implementation), CB u8, ...
 */
void SM83::D8() {
    fetched = read(pc++);
}

/**
 * Immediate byte to register addressing mode is used for instructions that have a register and an immediate byte.
 * Ex: LD A,u8, LD B,u8, ...
 */
void SM83::R_D8() {
    fetched = read(pc++);
}

/**
 * Immediate byte to memory referenced by register.
 * Ex: LD (HL),u8
 */
void SM83::MR_D8() {
    dstIsMem = true;
    memDest   = readReg(instruction->dstReg);
    fetched   = read(pc++);
}

/**
 * Immediate word addressing mode is used for instructions that have an immediate word
 * (typically branching instructions).
 * Ex: JP u16, CALL u16, ...
 */
void SM83::D16() {
    fetched = read(pc++);
    fetched |= (((uint16_t) read(pc++)) << 8);
}

/**
 * Immediate word to register addressing mode is used for instructions that have a register and an immediate word.
 * Ex: LD HL,d16, LD BC,d16, ...
 */
void SM83::R_D16() {
    fetched = read(pc++);
    fetched |= (((uint16_t) read(pc++)) << 8);
}

/**
 * Memory referenced by register.
 * Ex: INC (HL), DEC (HL)
 */
void SM83::MR() {
    dstIsMem = true;
    memDest   = readReg(instruction->dstReg);
    fetched   = read(memDest);
}

/**
 * Register to memory location referenced by register addressing mode is used for instructions that have a register
 * and a memory location referenced by a register.
 * Ex: LD (BC),A, LD (HL),B, ...
 */
void SM83::MR_R() {
    fetched   = readReg(instruction->srcReg);
    dstIsMem = true;
    memDest   = readReg(instruction->dstReg);
    if (instruction->dstReg == Register::C) // LD (C),A
        memDest |= 0xFF00;
}

/**
 * Memory location referenced by register to register addressing mode is used for instructions that have a register
 * and a memory location referenced by a register.
 * Ex: LD A,(HL), LD A,(BC), ...
 */
void SM83::R_MR() {
    temp16 = readReg(instruction->srcReg);
    if (instruction->srcReg == Register::C) // LD A,(C)
        temp16 |= 0xFF00;
    fetched = read(temp16);
}

/**
 * Byte at memory location referenced by HL to register; increment HL after.
 * Ex: LD A,(HL+)
 */
void SM83::R_HLI() {
    temp16  = readReg(Register::HL);
    fetched = read(temp16);
    writeReg(Register::HL, temp16 + 1);
}

/**
 * Register to memory location referenced by HL; increment HL after.
 * Ex: LD (HL+),A
 */
void SM83::HLI_R() {
    fetched   = readReg(instruction->srcReg);
    dstIsMem = true;
    memDest   = readReg(Register::HL);
    writeReg(Register::HL, readReg(Register::HL) + 1);
}

/**
 * Byte at memory location referenced by HL to register; decrement HL after.
 * Ex: LD A,(HL-)
 */
void SM83::R_HLD() {
    temp16  = readReg(Register::HL);
    fetched = read(temp16);
    writeReg(Register::HL, temp16 - 1);
}

/**
 * Register to memory location referenced by HL; decrement HL after.
 * Ex: LD (HL-),A
 */
void SM83::HLD_R() {
    fetched   = readReg(instruction->srcReg);
    dstIsMem = true;
    memDest   = readReg(Register::HL);
    writeReg(Register::HL, readReg(Register::HL) - 1);
}

/**
 * SP + s8 (signed byte) to HL register.
 * Ex: LD HL,SP+s8
 */
void SM83::HL_SPR() {
    fetched = read(pc++);
}


/**
 * Immediate unsigned byte to register addressing mode.
 * Ex: LDH A,(a8), which is the same as LD A,($FF00+a8)
 */
void SM83::R_A8() {
    temp16 = 0xFF00 | ((uint16_t) read(pc++));
    fetched = read(temp16);
}

/**
 * Register to memory referenced by immediate unsigned byte (0xFF00 + a8).
 * Ex: LDH (a8),A, which is the same as LD ($FF00+a8),A
 */
void SM83::A8_R() {
    dstIsMem = true;
    memDest   = 0xFF00 | ((uint16_t) read(pc++));
    fetched   = readReg(instruction->srcReg);
}

/**
 * Memory referenced by immediate word to register.
 * Ex: LD A,(u16)
 */
void SM83::R_A16() {
    temp16 = read(pc++);
    temp16 |= ((uint16_t) read(pc++) << 8);
    fetched = read(temp16);
}

/**
 * Register to memory referenced by immediate word.
 * Ex: LD (u16),A, LD (u16),SP
 */
void SM83::A16_R() {
    temp16 = read(pc++);
    temp16 |= (((uint16_t) read(pc++)) << 8);
    dstIsMem = true;
    memDest   = temp16;
    fetched   = readReg(instruction->srcReg);
}
// =====================================================================================================================


// Opcodes =============================================================================================================
// Here are the implementations of all the unique opcodes for the CPU. They (should) cover all 512 of them.
// The information on timing is from https://izik1.github.io/gbops/
// Although they pass Blargg's cpu_instrs and mem_timing tests, I am not 100% sure that they are all correct.
/**
 * Add register/memory/immediate to Accumulator with carry.
 * Flags: Z 0 H C
 * Timing (register)  (4t/1m): fetch                 Ex: ADC A,B
 * Timing (memory)    (8t/2m): fetch, read (HL)      Ex: ADC A,(HL)
 * Timing (immediate) (8t/2m): fetch, read u8        Ex: ADC A,u8
 */
void SM83::ADC() {
    // Since ADC operates only on 8-bit values, we perform the addition
    // in a 16-bit domain to capture the carry bit (bit 8) in the result.
    temp16 = a + fetched + getFlag(C);
    setFlag(Z, (temp16 & 0x00FF) == 0);
    setFlag(N, false);
    // The half-carry flag is set if there is a carry from bit 3 to bit 4.
    setFlag(H, (a & 0xF) + (fetched & 0xF) + getFlag(C) > 0xF);
    setFlag(C, temp16 > 0xFF);
    a = temp16 & 0xFF;
}

/**
 * Add register/memory/immediate to Accumulator, HL, or SP (without carry).
 * Flags: Z 0 H C
 * Timing (Accumulator) (4t/1m):  fetch                                     Ex: ADD A,B, ADD A,C, ...
 * Timing (16-bit HL)   (8t/2m):  fetch, write HL ???                       Ex: ADD HL,BC, ADD HL,DE, ...
 * Timing (memory)      (8t/2m):  fetch, read (HL)                          Ex: ADD A,(HL)
 * Timing (immediate)   (8t/2m):  fetch, read u8                            Ex: ADD A,u8
 * Timing (SP + s8)     (16t/4m): fetch, read u8, write SP:low ?, high?     Ex: ADD SP,s8
 *
 * For setting carry/half-carry flags for 16-bit ADD, see: https://stackoverflow.com/a/57981912
 */
void SM83::ADD() {
    dst = instruction->dstReg;
    setFlag(N, false);
    if (is16Bit(dst)) {
        temp16 = readReg(dst);
        if (dst == Register::SP) { // ADD SP,s8 (see opcode 0xE8)
            temp32 = temp16 + (int8_t) fetched; // signed (int8_t)
            setFlag(Z, false);
            setFlag(H, carry(temp16, (int8_t) fetched, temp32, 0x08)); // mask is bit 3
            setFlag(C, carry(temp16, (int8_t) fetched, temp32, 0x80)); // mask is bit 7
            writeReg(dst, temp32);
            emulateCycles(2); // gbops claims two internals (cycles verified by instr_timing): write SP:low, high ???
        } else { // ADD HL,BC ...
            // Adding 16-bit values in a 32-bit domain to capture the carry bit (bit 16) in the result.
            temp32 = (uint32_t) temp16 + (uint32_t) fetched;
            setFlag(H, carry(temp16, fetched, temp32, 0x800));   // from bit 11
            setFlag(C, carry(temp16, fetched, temp32, 0x8000));  // from bit 15
            writeReg(dst, temp32);
            emulateCycles(1);
        }
    } else { // ADD B ...
        temp8 = readReg(dst);
        temp16 = (uint16_t) temp8 + fetched;
        setFlag(Z, (temp16 & 0xFF) == 0);
        setFlag(H, carry(temp8, fetched, temp16, 0x08));
        setFlag(C, carry(temp8, fetched, temp16, 0x80));
        writeReg(dst, temp16);
    }
}

/**
 * Logical AND Accumulator with register/memory/immediate.
 * Flags: Z 0 1 0
 * Timing (4t/1m): fetch                 Ex: AND A,B, AND A,(HL), AND A,u8...
 */
void SM83::AND() {
    a &= fetched;
    setFlag(Z, a == 0);
    setFlag(N, false);
    setFlag(H, true);
    setFlag(C, false);
}

/**
 * Call subroutine at address.
 * Flags: ----
 * Timing without branch (12t/3m)       Timing with branch (24t/6m)
 * fetch,                               fetch,
 * read u16:lower,                      read u16:lower,
 * read u16:upper                       read u16:upper,
 *                                      internal (branch decision?),
 *                                      write PC:upper->(--SP),
 *                                      write PC:lower->(--SP)
 */
void SM83::CALL() {
    if (testCond(instruction->cond)) {
        emulateCycles(1);       // internal branch decision ???
        write(--sp, pc >> 8);
        write(--sp, pc & 0xFF);
        pc = fetched;
    }
}

/**
 * Complement (flip) carry flag.
 * Flags: - 0 0 !C
 * Timing (4t/1m): fetch
 */
void SM83::CCF() {
    setFlag(N, false);
    setFlag(H, false);
    setFlag(C, !getFlag(C));
}

/**
 * Complement Accumulator.
 * Flags: - 1 1 -
 * Timing (4t/1m): fetch
 */
void SM83::CPL() {
    a = ~a;
    setFlag(N, true);
    setFlag(H, true);
}

/**
 * Decimal adjust Accumulator. This instruction adjusts the result of the last
 * arithmetical operation to make it a valid BCD number.
 * TODO: I do not completely understand this.
 * Flags: Z - 0 C
 * Timing (4t/1m): fetch
 */
void SM83::DAA() {
    temp16 = 0;
    temp8 = getFlag(C);
    if (((a & 0x0F) > 0x09 && !getFlag(N)) || getFlag(H)) {
        temp16 = 0x06;
    }
    if ((a > 0x99 && !getFlag(N)) || getFlag(C)) {
        temp16 |= 0x60;
        temp8 = 1;
    }
    a += getFlag(N) ? -temp16 : temp16;
    setFlag(Z, a == 0);
    setFlag(H, false);
    setFlag(C, temp8);
}


/**
 * Compare Accumulator with register/memory/immediate; does not affect A.
 * Flags: Z 1 H C
 * Timing (register)  (4t/1m): fetch                 Ex: CP A,B, CP A,C, ...
 * Timing (memory)    (8t/2m): fetch, read (HL)      Ex: CP A,(HL)
 * Timing (immediate) (8t/2m): fetch, read u8        Ex: CP A,u8
 */
void SM83::CP() {
    temp8 = a - (fetched & 0xFF);
    setFlag(Z, temp8 == 0);
    setFlag(N, true);
    setFlag(H, borrow(a, fetched & 0xFF, temp8, 0x08));
    setFlag(C, borrow(a, fetched & 0xFF, temp8, 0x80));
}

/**
 * Decrement register/memory.
 * Flags (8-bit register or (HL)): Z 1 H -
 * Flags (16-bit register):        - - - -
 * Timing (8-bit register)  (4t/1m):  fetch                                         Ex: DEC B, DEC C, ...
 * Timing (memory (HL))     (12t/3m): fetch, read (HL), write (HL)                  Ex: DEC (HL)
 * Timing (16-bit register) (8t/2m):  fetch (write rr:low ???), write rr:high ???   Ex: DEC BC, DEC SP, ...
 */
void SM83::DEC() {
    dst = instruction->dstReg;
    if (is16Bit(dst)) {
        if (dstIsMem) { // DEC (HL)
            temp8 = fetched - 1;
            setFlag(Z, temp8 == 0);
            setFlag(N, true);
            setFlag(H, borrow(fetched, 1, temp8, 0x08));
            write(memDest, temp8);
        } else { // DEC BC ...
            writeReg(dst, fetched - 1);
            emulateCycles(1);
        }
    } else { // DEC B ...
        temp8 = fetched - 1;
        setFlag(Z, temp8 == 0);
        setFlag(N, true);
        setFlag(H, borrow(fetched, 1, temp8, 0x08));
        writeReg(dst, temp8);
    }
}

/**
 * Disable interrupt master enable (IME) flag.
 * Flags: ----
 * Timing (4t/1m): fetch
 */
void SM83::DI() {
    intHandler->IME = false;
    intHandler->scheduledIME = false;
}

/**
 * Enables the interrupt master enable (IME) flag the following cycle to its execution.
 * DI nor RETI have this sort of delay.
 * Flags: ----
 * Timing (4t/1m): fetch
 */
void SM83::EI() {
    intHandler->scheduledIME = true;
}

/**
 * Halt CPU until an interrupt occurs (IE & IF != 0).
 * Flags: ----
 * Timing (4t/1m): fetch
 */
void SM83::HALT() {
    halted = true;
}

/**
 * Increment register/memory.
 * Flags (8-bit register or (HL)): Z 0 H -
 * Flags (16-bit register):        - - - -
 * Timing (8-bit register)  (4t/1m):  fetch                               Ex: INC B, INC C, ...
 * Timing (memory (HL))     (12t/3m): fetch, read (HL), write (HL)        Ex: INC (HL)
 * Timing (16-bit register) (8t/2m):  fetch, write rr ???                 Ex: INC BC, INC SP, ...
 */
void SM83::INC() {
    dst = instruction->dstReg;
    if (is16Bit(dst)) {
        if (dstIsMem) { // INC (HL)
            temp8 = fetched + 1;
            setFlag(Z, temp8 == 0);
            setFlag(N, false);
            setFlag(H, carry(fetched, 1, temp8, 0x08));
            write(memDest, temp8);
        } else { // INC BC ...
            writeReg(dst, fetched + 1);
            emulateCycles(1);
        }
    } else { // INC B ...
        temp8 = fetched + 1;
        setFlag(Z, temp8 == 0);
        setFlag(N, false);
        setFlag(H, carry(fetched, 1, temp8, 0x08));
        writeReg(dst, temp8);
    }
}

/**
 * Jump to address.
 * Flags: ----
 *
 * For JP Z, u16, JP NC,u16, ...
 * Timing without branch (12t/3m): fetch, read u16:lower, read u16:upper
 * Timing with branch    (16t/4m): fetch, read u16:lower, read u16:upper, internal (branch decision?)
 *
 * For JP HL
 * Timing (4t/1m): fetch
 *
 * For JP u16 (unconditional)
 * Timing (16t/4m): fetch, read u16:lower, read u16:upper, internal (branch decision?) (but there is no condition ???)
 */
void SM83::JP() {
    if (testCond(instruction->cond)) { // JP Z, JP NC, ... (JP defaults to true)
        pc = fetched;
        if (instruction->addrmode != &SM83::R) // JP HL (see docstring)
            emulateCycles(1); // internal branch decision ???
    }
}

/**
 * Jump relative (i.e., jump s8 steps from the current address in the program counter)
 * Flags: ----
 *
 * For JR Z,s8, JR NC,s8, ...
 * Timing without branch (8t/2m): fetch, read s8
 * Timing with branch    (12t/3m): fetch, read s8, internal (modify PC or branch decision or both?)
 *
 * For JR s8 (unconditional)
 * Timing (12t/3m): fetch, read s8, internal (modify PC)
 */
void SM83::JR() {
    if (testCond(instruction->cond)) { // JR Z, JR NC, ... (JR defaults to true)
        pc += static_cast<int8_t>(fetched & 0xFF); // s8 is signed (int8_t)
        emulateCycles(1);
    }
}

/**
 * Load: Read from register/memory and write to register/memory.
 * Note: In the notation below, (rr) means the memory location referenced by the 16-bit register pair rr.
 * Flags: ---- (except for LD HL,SP+s8, see below)
 * Timing (register to register) (4t/1m):  fetch                            Ex: LD A,B, LD B,C, ...
 * Timing (register to (rr))     (8t/2m):  fetch, write memory              Ex: LD (HL),B, LD (HL),C, ...
 * Timing (register to (u16))    (16t/4m): fetch, read u16:lower,
 *                                         read u16:upper, write (u16)      Ex: LD (u16),A
 * Timing ((rr) to register)     (8t/2m):  fetch, read (rr)                 Ex: LD A,(HL), LD B,(HL), ...
 * Timing LD HL,SP+s8            (12t/3m): fetch, read s8, internal delay     Ex: LD HL,SP+s8
 *      - Flags: 0 0 H C
 * Timing LD HL,SP               (8t/2m): fetch, internal delay               Ex: LD HL,SP
 * Timing LD (u16),SP            (20t/5m): fetch,
 *                                         read u16:lower, read u16:upper,
 *                                         write SP:lower->(u16), write SP:upper->(u16+1)
 */
void SM83::LD() {
    dst = instruction->dstReg;
    src = instruction->srcReg;
    if (dstIsMem) {
        if (is16Bit(src)) { // LD (u16),SP (see opcode 0x08)
            write(memDest + 0, fetched & 0xFF);
            write(memDest + 1, fetched >> 8);
        } else { // LD (C),A, LD (HL),u8 ...
            write(memDest, fetched);
        }
    } else if (instruction->addrmode == &SM83::HL_SPR) { // LD HL,SP+s8 (see opcode 0xF8)
        temp32 = (uint32_t) sp + (int8_t) fetched; // signed (int8_t)
        setFlag(Z, false);
        setFlag(N, false);
        // Source (for updating carry and half-carry flags): https://stackoverflow.com/a/7261149
        if (((int8_t) fetched) >= 0) {
            setFlag(H, carry(sp, (int8_t) fetched, temp32, 0x08));
            setFlag(C, carry(sp, (int8_t) fetched, temp32, 0x80));
        } else {
            setFlag(H, (temp32 & 0x0F) <= (sp & 0x0F));
            setFlag(C, (temp32 & 0xFF) <= (sp & 0xFF));
        }
        writeReg(dst, (uint16_t) temp32);
        emulateCycles(1); // internal delay
    } else { // LD B,C, ..., LD DE,u16, ..., LD A,(DE), ..., LD E,u8, ...
        writeReg(dst, fetched);
        if (dst == Register::SP && src == Register::HL) // LD SP,HL (see opcode 0xF9)
            emulateCycles(1); // internal delay
    }
}

/**
 * Load: Read from or write to I/O-port n (memory 0xFF00 + n)
 * Flags: ----
 * Timing (writing to memory) (12t/3m): fetch, read u8, write A->(FF00+u8)      Ex: LD ($FF00+u8),A
 * Timing (writing to A) (12t/3m):      fetch, read u8, read (FF00+u8)->A       Ex: LD A,($FF00+u8)
 *
 * @note see R_A8 and A8_R addressing modes for more details.
 */
void SM83::LDH() {
    if (dstIsMem)
        write(memDest, fetched);
    else
        writeReg(instruction->dstReg, fetched);
}

/**
 * No operation. Doesn't do anything, but can be used to add a delay
 * of one machine cycle and increment the program counter by 1.
 * Flags: ----
 * Timing (4t/1m): fetch
 */
void SM83::NOP() {}

/**
 * Pop contents of memory stack into register pair.
 * Flags: ----
 * Timing (12t/3m): fetch, read (SP++)->lower, read (SP++)->upper
 *
 * @note SP is incremented when popping.
 */
void SM83::POP() {
    temp16 = read(sp++);                      // lo
    temp16 |= (((uint16_t) read(sp++)) << 8); // hi
    dst = instruction->dstReg;
    if (dst == Register::AF)
        // Flag bits 0,1,2,3 are always zero
        temp16 &= 0xFFF0;
    writeReg(dst, temp16);
}

/**
 * Push contents of register pair onto memory stack.
 * Flags: ----
 * Timing without branch (16t/4m): fetch, internal delay, write Hi->(--SP), write Lo->(--SP)
 *
 * @note SP is decremented when pushing.
 */
void SM83::PUSH() {
    emulateCycles(1);            // internal delay
    write(--sp, fetched >> 8);   // hi
    write(--sp, fetched & 0xFF); // lo
}

/**
 * Return from a subroutine.
 * Flags: ----
 * For RET Z, RET NZ, ...
 * Timing without branch (8t/2m)	with branch (20t/5m)
 * fetch	                        fetch
 * internal (branch decision?)	    internal (branch decision?)
 *                                  read (SP++)->lower
 *                                  read (SP++)->upper
 *                                  internal (set PC?)
 *
 * For RET (where condition is always true, i.e., instruction->cond == Condition::X),
 * Timing with branch (16t/4m): fetch, read (SP++)->lower, read (SP++)->upper, internal (set PC?)
 */
void SM83::RET() {
    if (instruction->cond != Condition::X)
        emulateCycles(1); // conditional returns all require internal branch decision check

    if (testCond(instruction->cond)) {
        temp16 = read(sp++);
        temp16 |= (((uint16_t) read(sp++)) << 8);
        pc = temp16;
        emulateCycles(1); // internal (set PC?)
    }
}

/**
 * Return from a subroutine and enable interrupts (return from interrupt handler).
 * Flags: ----
 * Timing is same as RET without internal branch decision check (16t/4m).
 */
void SM83::RETI() {
    RET();
    intHandler->IME = true;
}

/**
 * Rotate Accumulator left through carry.
 * Flags: 0 0 0 A7
 * Timing (4t/1m): fetch
 */
void SM83::RLA() {
    temp8 = a >> 7;            // save msb
    a = (a << 1) | getFlag(C); // old carry is new lsb
    setFlag(Z, false);
    setFlag(N, false);
    setFlag(H, false);
    setFlag(C, temp8);         // carry gets old msb
}

/**
 * Rotate Accumulator left (not through carry).
 * Flags: 0 0 0 A7
 * Timing (4t/1m): fetch
 */
void SM83::RLCA() {
    temp8 = a >> 7;
    a = (a << 1) | temp8;
    setFlag(Z, false);
    setFlag(N, false);
    setFlag(H, false);
    setFlag(C, temp8);
}

/**
 * Rotate Accumulator right through carry.
 * Flags: 0 0 0 A0
 * Timing (4t/1m): fetch
 */
void SM83::RRA() {
    temp8 = a & 1;                      // save lsb
    a = (getFlag(C) << 7) | (a >> 1);   // old carry is new msb
    setFlag(Z, false);
    setFlag(N, false);
    setFlag(H, false);
    setFlag(C, temp8);                 // carry gets old lsb
}

/**
 * Rotate Accumulator right (not through carry).
 * Flags: 0 0 0 A0
 * Timing (4t/1m): fetch
 */
void SM83::RRCA() {
    temp8 = a & 1;
    a = (temp8 << 7) | (a >> 1);
    setFlag(Z, false);
    setFlag(N, false);
    setFlag(H, false);
    setFlag(C, temp8);
}

/**
 * Restart at address. The RST instruction can be used to jump to one of eight addresses.
 * Flags: ----
 * Timing (16t/4m): fetch, internal delay, write PC:upper->(--SP), write PC:lower->(--SP)
 */
void SM83::RST() {
    emulateCycles(1);       // internal delay
    write(--sp, pc >> 8);   // hi
    write(--sp, pc & 0xFF); // lo
    pc = instruction->param;
}

/**
 * Subtract register/memory/immediate from Accumulator with carry.
 * Flags: Z 1 H C
 * Timing (register)  (4t/1m): fetch                 Ex: SBC A,B, SBC A,C, ...
 * Timing (memory)    (8t/2m): fetch, read (HL)      Ex: SBC A,(HL)
 * Timing (immediate) (8t/2m): fetch, read u8        Ex: SBC A,u8
 */
void SM83::SBC() {
    temp8 = a - ((fetched & 0xFF) + getFlag(C));
    setFlag(Z, temp8 == 0);
    setFlag(N, true);
    setFlag(H, (a & 0x0F) < (fetched & 0x0F) + getFlag(C));
    setFlag(C, a < (fetched & 0xFF) + getFlag(C));
    a = temp8;
}

/**
 * Set carry flag.
 * Flags: - 0 0 1
 * Timing (4t/1m): fetch
 */
void SM83::SCF() {
    setFlag(N, false);
    setFlag(H, false);
    setFlag(C, true);
}

/**
 * Stop.
 * TODO: Implement this.
 * Flags: ----
 * Timing (4t/1m): fetch
 */
void SM83::STOP() {
    std::cerr << "STOPPING!\n" << std::endl;
}

/**
 * Subtract register/memory/immediate from Accumulator.
 * Flags: Z 1 H C
 * Timing (register)  (4t/1m): fetch                 Ex: SUB A,B, SUB A,C, ...
 * Timing (memory)    (8t/2m): fetch, read (HL)      Ex: SUB A,(HL)
 * Timing (immediate) (8t/2m): fetch, read u8        Ex: SUB A,u8
 */
void SM83::SUB() {
    temp8 = a - (fetched & 0xFF);
    setFlag(Z, temp8 == 0);
    setFlag(N, true);
    setFlag(H, borrow(a, fetched & 0xFF, temp8, 0x08));
    setFlag(C, borrow(a, fetched & 0xFF, temp8, 0x80));
    a = temp8;
}

/**
 * Logical OR Accumulator with register/memory/immediate.
 * Flags: Z 0 0 0
 * Timing (register)  (4t/1m): fetch                 Ex: OR A,B, OR A,C, ...
 * Timing (memory)    (8t/2m): fetch, read (HL)      Ex: OR A,(HL)
 * Timing (immediate) (8t/2m): fetch, read u8        Ex: OR A,u8
 */
void SM83::OR() {
    a |= fetched;
    setFlag(Z, a == 0);
    setFlag(N, false);
    setFlag(H, false);
    setFlag(C, false);
}

/**
 * Logical XOR Accumulator with register/memory/immediate.
 * Flags: Z 0 0 0
 * Timing (register)  (4t/1m): fetch                 Ex: XOR A,B, XOR A,C, ...
 * Timing (memory)    (8t/2m): fetch, read (HL)      Ex: XOR A,(HL)
 * Timing (immediate) (8t/2m): fetch, read u8        Ex: XOR A,u8
 */
void SM83::XOR() {
    a ^= fetched;
    setFlag(Z, a == 0);
    setFlag(N, false);
    setFlag(H, false);
    setFlag(C, false);
}

/**
 * Captures all unrecognized instructions.
 */
void SM83::XXX() {
    printf("UNRECOGNIZED INSTRUCTION @ %04Xh: %02X\n", pc - 1, opcode);
}

/* CB-Prefixed Instructions */

/**
 * Handles CB-prefixed instructions.
 *
 * CB-prefixed instructions are encoded like so:
 * Bits in opcode (MSB -> LSB):     xx yyy zzz
 * A CB-prefixed instruction (the immediate byte that is fetched after CB is called) is encoded like so:
 *     x = 0 -> rot[y]r[z] : Roll/shift register or memory location (HL), where the lowest three bits of the
 *                           opcode (zzz) map to the registers in the vector below. rot corresponds to cbLookup.
 *     x = 1 -> BIT y, r[z]: Test bit y of registers[z]
 *     x = 2 -> RES y, r[z]: Reset bit y of registers[z]
 *     x = 3 -> SET y, r[z]: Set bit y of registers[z]
 *
 * Base Timing (8t/1m): fetch instruction, read u8 (immediate byte after CB to be decoded)
 * Reading from (HL) adds 4t/1m to the timing. Writing back to (HL) adds another 4t/1m to the timing.
 */
void SM83::CB() {
    using r = SM83::Register;
    // Order is important here for decoding CB-prefixed opcodes. See table "r" in the link below.
    // https://gb-archive.github.io/salvage/decoding_gbz80_opcodes/Decoding%20Gamboy%20Z80%20Opcodes.html
    //                                               0     1     2     3     4     5     6      7
    static constexpr std::array<r, 8> registers = { r::B, r::C, r::D, r::E, r::H, r::L, r::HL, r::A };

    // `fetched` holds the value of the next immediate byte to be decoded for a CB-prefixed instruction
    xx  = (fetched >> 6) & 0b11;  // Operation to perform
    yyy = (fetched >> 3) & 0b111; // Bit for BIT/RES/SET, or index for cbLookup
    zzz = fetched & 0b111;        // Index for register lookup

    dst = registers[zzz];
    if (dst == Register::HL) { // BIT y,(HL), RES y,(HL), SET y,(HL) ...
        dstIsMem = true;
        memDest   = readReg(dst);
        fetched   = read(readReg(dst));
    } else {
        fetched = readReg(dst);
    }

    switch (xx) {
        case 0: (this->*cbLookup[yyy].operate)(); return;
        case 1: BIT();                            return;
        case 2: RES();                            return;
        case 3: SET();                            return;
        default:                                  return;
    }
}

/**
 * Test bit yyy of registers[zzz] or (HL).
 * Flags: Z 0 1 -
 * Timing (HL)       (12t/3m): fetch, fetch, read (HL)
 * Timing (register) (8t/2m):  fetch, fetch
 */
void SM83::BIT() {
    setFlag(Z, !(fetched & (1 << yyy)));
    setFlag(N, false);
    setFlag(H, true);
}

/**
 * Reset bit yyy of registers[zzz] or (HL).
 * Flags: ----
 * Timing (HL)       (16t/4m): fetch, fetch, read (HL), write (HL)
 * Timing (register) (8t/2m):  fetch, fetch
 */
void SM83::RES() {
    temp8 = fetched & ~(1 << yyy);
    if (dstIsMem)
        write(memDest, temp8);
    else
        writeReg(dst, temp8);
}

/**
 * Set bit yyy of registers[zzz] or (HL).
 * Flags: ----
 * Timing (HL)       (16t/4m): fetch, fetch, read (HL), write (HL)
 * Timing (register) (8t/2m):  fetch, fetch
 */
void SM83::SET() {
    temp8 = fetched | (1 << yyy);
    if (dstIsMem)
        write(memDest, temp8);
    else
        writeReg(dst, temp8);
}

/* Rotate/shift register or memory location CB-prefixed instructions */

/**
 * Rotate register/memory left.
 * Flags: Z 0 0 C
 * Timing (HL)       (16t/4m): fetch, fetch, read (HL), write (HL)
 * Timing (register) (8t/2m):  fetch, fetch
 */
void SM83::RLC() {
    temp8 = (fetched << 1) | (fetched >> 7);
    if (dstIsMem)
        write(memDest, temp8);
    else
        writeReg(dst, temp8);

    setFlag(Z, temp8 == 0);
    setFlag(N, false);
    setFlag(H, false);
    setFlag(C, (fetched >> 7) & 1);
}

/**
 * Rotate register/memory right.
 * Flags: Z 0 0 C
 * Timing (HL)       (16t/4m): fetch, fetch, read (HL), write (HL)
 * Timing (register) (8t/2m):  fetch, fetch
 */
void SM83::RRC() {
    temp8 = (fetched << 7) | (fetched >> 1);
    if (dstIsMem)
        write(memDest, temp8);
    else
        writeReg(dst, temp8);

    setFlag(Z, temp8 == 0);
    setFlag(N, false);
    setFlag(H, false);
    setFlag(C, fetched & 1);
}

/**
 * Rotate register/memory left through carry.
 * Flags: Z 0 0 C
 * Timing (HL)       (16t/4m): fetch, fetch, read (HL), write (HL)
 * Timing (register) (8t/2m):  fetch, fetch
 */
void SM83::RL() {
    temp8 = (fetched << 1) | getFlag(C);
    if (dstIsMem)
        write(memDest, temp8);
    else
        writeReg(dst, temp8);

    setFlag(Z, temp8 == 0);
    setFlag(N, false);
    setFlag(H, false);
    setFlag(C, (fetched >> 7) & 1);
}

/**
 * Rotate register/memory right through carry.
 * Flags: Z 0 0 C
 * Timing (HL)       (16t/4m): fetch, fetch, read (HL), write (HL)
 * Timing (register) (8t/2m):  fetch, fetch
 */
void SM83::RR() {
    temp8 = (getFlag(C) << 7) | (fetched >> 1);
    if (dstIsMem)
        write(memDest, temp8);
    else
        writeReg(dst, temp8);

    setFlag(Z, temp8 == 0);
    setFlag(N, false);
    setFlag(H, false);
    setFlag(C, fetched & 1);
}

/**
 * Shift left arithmetic register/memory through carry.
 * Flags: Z 0 0 C
 * Timing (HL)       (16t/4m): fetch, fetch, read (HL), write (HL)
 * Timing (register) (8t/2m):  fetch, fetch
 */
void SM83::SLA() {
    temp8 = fetched << 1;
    if (dstIsMem)
        write(memDest, temp8);
    else
        writeReg(dst, temp8);

    setFlag(Z, temp8 == 0);
    setFlag(N, false);
    setFlag(H, false);
    setFlag(C, (fetched >> 7) & 1);
}

/**
 * Shift right arithmetic register/memory (sign bit perserved).
 * Flags: Z 0 0 C
 * Timing (HL)       (16t/4m): fetch, fetch, read (HL), write (HL)
 * Timing (register) (8t/2m):  fetch, fetch
 */
void SM83::SRA() {
    temp8 = ((int8_t) fetched) >> 1; // preserve the sign bit (int8_t)
    if (dstIsMem)
        write(memDest, temp8);
    else
        writeReg(dst, temp8);

    setFlag(Z, temp8 == 0);
    setFlag(N, false);
    setFlag(H, false);
    setFlag(C, fetched & 1);
}

/**
 * Swap high and low nibbles of register/memory.
 * Flags: Z 0 0 0
 * Timing (HL)       (16t/4m): fetch, fetch, read (HL), write (HL)
 * Timing (register) (8t/2m):  fetch, fetch
 */
void SM83::SWAP() {
    temp8 = (fetched << 4) | (fetched >> 4);
    if (dstIsMem)
        write(memDest, temp8);
    else
        writeReg(dst, temp8);

    setFlag(Z, temp8 == 0);
    setFlag(N, false);
    setFlag(H, false);
    setFlag(C, false);
}

/**
 * Shift right logical register/memory (sign bit is not preserved).
 * Flags: Z 0 0 C
 * Timing (HL)       (16t/4m): fetch, fetch, read (HL), write (HL)
 * Timing (register) (8t/2m):  fetch, fetch
 */
void SM83::SRL() {
    temp8 = fetched >> 1;
    if (dstIsMem)
        write(memDest, temp8);
    else
        writeReg(dst, temp8);

    setFlag(Z, temp8 == 0);
    setFlag(N, false);
    setFlag(H, false);
    setFlag(C, fetched & 1);
}
// =====================================================================================================================

#if LOGGING // Set true in SM83.h to enable logging
/**
 * Helper function to convert a register enum to a string (for disassembly).
 */
std::string SM83::registerToString(Register r) {
    switch (r) {
        case Register::A: return "A";
        case Register::B: return "B";
        case Register::C: return "C";
        case Register::D: return "D";
        case Register::E: return "E";
        case Register::F: return "F";
        case Register::H: return "H";
        case Register::L: return "L";
        case Register::AF: return "AF";
        case Register::BC: return "BC";
        case Register::DE: return "DE";
        case Register::HL: return "HL";
        case Register::SP: return "SP";
        default: return "";
    }
}

/**
 * Helper function to convert a condition enum to a string (for disassembly)
 */
std::string SM83::conditionToString(SM83::Condition cc) {
    switch (cc) {
        case Condition::NZ: return "NZ";
        case Condition::Z : return "Z";
        case Condition::NC: return "NC";
        case Condition::C : return "C";
        default:            return "";
    }
}

/**
 * Convert instruction to readable assembly. Used for debugging. Pretty hacky, I know.
 */
void SM83::disassemble() {
    printf("%-9llu", logTicks);
    std::string sInst = instruction->name + " ";

    // For CALL C, JR NZ, ...
    if (instruction->cond != Condition::X)
        sInst += conditionToString(instruction->cond) + " ";

    // "A convenient utility to convert variables into hex strings because "modern C++"'s method with streams is atrocious." - OLC
    // Source: https://github.com/OneLoneCoder/olcNES/blob/ac5ce64cdb3a390a89d550c5f130682b37eeb080/Part%232%20-%20CPU/olc6502.cpp#L1479
    auto hex = [](uint32_t n, uint8_t len) {
        std::string s(len, '0');
        for (int i = len - 1; i >= 0; i--, n >>= 4)
            s[i] = "0123456789ABCDEF"[n & 0xF];
        return s;
    };

    using s = SM83;
    auto addrMode = instruction->addrmode;
    dst = instruction->dstReg;
    src = instruction->srcReg;
    if (addrMode == &s::IMP)
        sInst += "";
    else if (addrMode == &s::R_D16)
        sInst += registerToString(dst) + ",$" + hex(fetched, 4);
    else if (addrMode == &s::R_A16)
        sInst += registerToString(dst) + ",$" + hex(memDest, 4);
    else if (addrMode == &s::R)
        sInst += registerToString(dst);
    else if (addrMode == &s::R_R)
        sInst += registerToString(dst) + "," + registerToString(src);
    else if (addrMode == &s::MR_R)
        sInst += "(" + registerToString(dst) + ")," + registerToString(src);
    else if (addrMode == &s::MR)
        sInst += "(" + registerToString(dst) + ")";
    else if (addrMode == &s::R_MR)
        sInst += registerToString(dst) + ",(" + registerToString(src) + ")";
    else if (addrMode == &s::R_D8)
        sInst += registerToString(dst) + ",$" + hex(fetched & 0xFF, 2);
    else if (addrMode == &s::R_A8)
        sInst += registerToString(dst) + ",($" + hex(memDest, 4) + ")";
    else if (addrMode == &s::R_HLI)
        sInst += registerToString(dst) + ",(" + registerToString(src) + "+)";
    else if (addrMode == &s::R_HLD)
        sInst += registerToString(dst) + ",(" + registerToString(src) + "-)";
    else if (addrMode == &s::HLI_R)
        sInst += "(" + registerToString(dst) + "+)," + registerToString(src);
    else if (addrMode == &s::HLD_R)
        sInst += "(" + registerToString(dst) + "-)," + registerToString(src);
    else if (addrMode == &s::A8_R)
        sInst += "($" + hex(memDest, 4) + ")," + registerToString(src);
    else if (addrMode == &s::HL_SPR)
        sInst += "(" + registerToString(dst) + "),SP+" + hex(fetched & 0xFF, 2);
    else if (addrMode == &s::D8)
        sInst += "$" + hex(fetched & 0xFF, 2);
    else if (addrMode == &s::D16) {
        sInst += "$" + hex(fetched, 4);
    } else if (addrMode == &s::MR_D8)
        sInst += "(" + registerToString(dst) + "),$" + hex(fetched & 0xFF, 2);
    else if (addrMode == &s::A16_R)
        sInst += "($" + hex(memDest, 4) + ")," + registerToString(src);
    else
        std::cerr << "INVALID ADDRESSING MODE.\n";

    printf(
        "%04X: %-14s (op: %02X, %02X %02X)\tA: %02X,  BC: %02X%02X,  DE: %02X%02X,  HL: %02X%02X",
       logPC, sInst.c_str(),
       opcode, bus->read(logPC + 1), bus->read(logPC + 2),
       a, b, c, d, e, h, l
    );

    printf(
        "\tF: [%c%c%c%c]\n",
       getFlag(Z) ? 'Z' : '-',
       getFlag(N) ? 'N' : '-',
       getFlag(H) ? 'H' : '-',
       getFlag(C) ? 'C' : '-'
    );

    if (!instruction) {
        printf("UNKNOWN INSTRUCTION. %02X\n", opcode);
        exit(-7);
    }

    // All Blaggg's tests output to the serial port:
    // FF02 — SC: Serial transfer control
    // See: https://gbdev.io/pandocs/Serial_Data_Transfer_(Link_Cable).html#ff02--sc-serial-transfer-control
//    if (bus->read(0xFF02) == 0x81) { // MSB and LSB are set
//        debugMessage += static_cast<char>(bus->read(0xFF01));
//        bus->write(0xFF02, 0);
//    }
    printf("                                               	"
           "DIV ($FF04): %04X,  TIMA ($FF05): %02X,   TMA ($FF06): %02X,   TAC ($FF07): %02X\n", timer->sysClock, timer->tima, timer->tma, timer->tac);

//    if (!debugMessage.empty())
//        printf("DEBUG MESSAGE: %s\n\n", debugMessage.c_str());

}
#endif
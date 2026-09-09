#pragma once

#include <cstdint>
#include "enums.h"

namespace lc3kit::vm
{
    /**
     * @brief LC-3 opcode set implemented by the VM.
     */
    enum class opcodes : uint8_t {
        BR       = 0b0000,  // branch
        ADD      = 0b0001,
        LD       = 0b0010,  // load
        ST       = 0b0011,  // store
        JSR      = 0b0100,  // also JSRR depending on bit[11]
        AND      = 0b0101,
        LDR      = 0b0110,  // load register (base + offset)
        STR      = 0b0111,  // store register (base + offset)
        RTI      = 0b1000,  // return from interrupt
        NOT      = 0b1001,
        LDI      = 0b1010,  // load indirect
        STI      = 0b1011,  // store indirect
        JMP      = 0b1100,  // also RET when SR1 == R7
        EXT      = 0b1101,  // lc3kit Extension: SHL, SHR, MUL, DIV (need to be enabled)
        LEA      = 0b1110,
        TRAP     = 0b1111
    };

    /**
     * @brief General-purpose and special register identifiers.
     */
    enum class registers : uint8_t {
        R0 = 0b000,
        R1 = 0b001,
        R2 = 0b010,
        R3 = 0b011,
        R4 = 0b100,
        R5 = 0b101,
        R6 = 0b110,  // conventionally used as the stack pointer (SP)
        R7 = 0b111,  // conventionally used as the link register (return address for JSR/JSRR)
        PC,          // program counter — not part of the 3-bit GPR field
        PSR,         // processor status register — holds N/Z/P + privilege + priority
        _R_COUNT
    };

    /**
     * @brief Condition flags used by branch instructions.
     */
    enum class r_cond : uint8_t {
        POS = 0b001,  // P — last result was positive
        ZRO = 0b010,  // Z — last result was zero
        NEG = 0b100   // N — last result was negative
    };

    /**
     * @brief Memory-mapped I/O register addresses.
     */
    enum class mmio : std_word_t {
        KBSR = 0xFE00,  // keyboard status register
        KBDR = 0xFE02,  // keyboard data register
        DSR  = 0xFE04,  // display status register
        DDR  = 0xFE06,  // display data register
        MCR  = 0xFE08   // machine control register
    };

    /**
     * @brief Standard LC-3 trap vector dispatch values.
     */
    enum class trap_vectors : uint8_t {
        GETC = 0x20,  // read a character from the keyboard, no echo
        OUT  = 0x21,  // write a character to the console
        PUTS = 0x22,  // write a null-terminated string to the console
        IN   = 0x23,  // prompt for input, read a character, echo it
        PUTSP = 0x24, // write a null-terminated string of packed characters to the console
        HALT = 0x25   // halt execution and return control to the OS
    };

    /**
     * @brief Interrupt source identifiers.
     */
    enum class interrupts : uint8_t {
        KEYBOARD = 0x80,  // keyboard interrupt
    };

    /**
     * @brief Additional arithmetic and bitwise operations provided by the lc3kit extension.
     */
    enum class ext_opcodes : uint8_t {
        SHL = 0b00,  // shift left  — dr = sr1 << (imm3 | sr2)
        SHR = 0b01,  // shift right — dr = sr1 >> (imm3 | sr2), arithmetic
        MUL = 0b10,  // multiply    — dr = sr1 *  (imm3 | sr2)
        DIV = 0b11   // divide      — dr = sr1 /  (imm3 | sr2), halts on div-by-zero
    };

    /**
     * @brief Address range descriptor for a memory section.
     */
    struct section_addr {
        std_word_t origin;
        std_word_t size;
    };

    /**
     * @brief Collection of memory section address descriptors.
     */
    using sections_addr = std::vector<section_addr>;

} // namespace lc3kit

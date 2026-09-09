#pragma once

#include "lc3kit/common.h"
#include <unordered_map>

namespace lc3kit::lasm
{
    /**
     * @brief Token kinds produced by the lexer and consumed by the parser.
     */
    enum class token_type {
        COMMA,  // separate operands

        IDENTIFIER,  // register, label
        STRING,

        HEX_NUMBER,
        DEC_NUMBER,

        // arithmetic / logic
        ADD, AND, NOT,

        // branch
        BR, BRN, BRZ, BRP, BRNZ, BRNP, BRZP, BRNZP,

        // data movement
        LD, LDI, LDR, LEA, ST, STI, STR,

        // control flow
        JMP, JSR, JSRR, RET, RTI,

        // trap
        TRAP,

        // trap aliases
        GETC, OUT, PUTS, IN, PUTSP, HALT,

        // lc3kit-ext
        SHL, SHR, MUL, DIV,

        // directives
        ORIG, END, FILL, BLKW, STRINGZ,

        EOL,    // end of line `\n`
        EOF_,    // end of file

        INVALID
    };

    /**
     * @brief General-purpose register identifiers used by the assembler.
     */
    enum class registers {
        R0 = 0,
        R1,
        R2,
        R3,
        R4,
        R5,
        R6,
        R7,
        NOR
    };
} // namespace lc3kit::lasm

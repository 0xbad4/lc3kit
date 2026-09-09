#pragma once

#include "enums.h"

namespace lc3kit::lasm
{
    /**
     * @brief End-of-line marker used by the lexer and parser.
     */
    const constexpr char END_OF_LINE = '\n';

    /**
     * @brief Source position in a text file, in line and column form.
     */
    struct tpos {
        uint32_t line = 0;
        uint32_t col = 0;
    };

    /**
     * @brief Raw source buffer and its byte length.
     */
    struct source_code {
        const char* data;
        uint32_t    size = 0;
    };

    /**
     * @brief Map of standard LC-3 mnemonic names to token identifiers.
     */
    inline const std::unordered_map<str_t, token_type> MNEMONICS = {
        {"ADD",   token_type::ADD},
        {"AND",   token_type::AND},
        {"NOT",   token_type::NOT},

        {"BR",    token_type::BR},
        {"BRN",   token_type::BRN},
        {"BRZ",   token_type::BRZ},
        {"BRP",   token_type::BRP},
        {"BRNZ",  token_type::BRNZ},
        {"BRNP",  token_type::BRNP},
        {"BRZP",  token_type::BRZP},
        {"BRNZP", token_type::BRNZP},

        {"LD",    token_type::LD},
        {"LDI",   token_type::LDI},
        {"LDR",   token_type::LDR},
        {"LEA",   token_type::LEA},
        {"ST",    token_type::ST},
        {"STI",   token_type::STI},
        {"STR",   token_type::STR},

        {"JMP",   token_type::JMP},
        {"JSR",   token_type::JSR},
        {"JSRR",  token_type::JSRR},
        {"RET",   token_type::RET},
        {"RTI",   token_type::RTI},

        {"TRAP",  token_type::TRAP},

        {"GETC",  token_type::GETC},
        {"OUT",   token_type::OUT},
        {"PUTS",  token_type::PUTS},
        {"IN",    token_type::IN},
        {"PUTSP", token_type::PUTSP},
        {"HALT",  token_type::HALT},
    };

    inline const std::unordered_map<str_t, token_type> MNEMONICS_EXT {
        {"SHL",   token_type::SHL},
        {"SHR",   token_type::SHR},
        {"MUL",   token_type::MUL},
        {"DIV",   token_type::DIV}
    };

    inline const std::unordered_map<str_t, token_type> DIRECTIVES = {
        {"ORIG",    token_type::ORIG},
        {"END",     token_type::END},
        {"FILL",    token_type::FILL},
        {"BLKW",    token_type::BLKW},
        {"STRINGZ", token_type::STRINGZ}
    };

    // map every BR mnemonic variant to its 3-bit NZP condition mask.
    // parser constructs InsBR with the correct mask.
    inline const std::unordered_map<std::string, uint8_t> BR_MASKS = {
        {"BR",    0b111},
        {"BRNZP", 0b111},
        {"BRN",   0b100},
        {"BRZ",   0b010},
        {"BRP",   0b001},
        {"BRNZ",  0b110},
        {"BRNP",  0b101},
        {"BRZP",  0b011},
        {"JNZP",  0b111},
        {"JN",    0b100},
        {"JZ",    0b010},
        {"JP",    0b001},
        {"JNZ",   0b110},
        {"JNP",   0b101},
        {"JZP",   0b011},
        {"JNEVER",0b000},
    };

    // map trap alias mnemonics to their 8-bit trap vectors.
    // parser resolves aliases before constructing InsTRAP.
    inline const std::unordered_map<std::string, std_word_t> TRAP_VECTORS = {
        {"GETC",  0x20},
        {"OUT",   0x21},
        {"PUTS",  0x22},
        {"IN",    0x23},
        {"PUTSP", 0x24},
        {"HALT",  0x25},
    };

} // namespace lc3kit::lasm

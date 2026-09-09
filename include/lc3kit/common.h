#pragma once

#include <cstdint>
#include <string>
#include <bitset>
#include <vector>


namespace lc3kit {
    /**
     * @brief Maximum addressable LC-3 memory location.
     *
     * LC-3 uses a 16-bit address space, so valid addresses range from 0x0000 to 0xFFFF.
     */
    const constexpr uint16_t MEMORY_MAX = 0xFFFF;  // 65536 unit

    /**
     * @brief Base address of the interrupt vector table.
     */
    const constexpr uint16_t INTERRUPT_VEC_BASE_ADDR = 0x0100;   // interrupt vector base addr

    /**
     * @brief 16-bit machine word type used throughout the toolkit.
     */
    using std_word_t    = uint16_t;

    /**
     * @brief Standard string alias for convenience.
     */
    using str_t         = std::string;

    /**
     * @brief Signed 16-bit integer type.
     */
    using std_sword_t   = int16_t;

    /**
     * @brief Bitset used to track breakpoint locations in memory.
     */
    using breakpoints_t = std::bitset<MEMORY_MAX>;

    /**
     * @brief Extract an inclusive bit field [hi:lo] and right-align it.
     *
     * @param src Source word.
     * @param hi Upper bit index, inclusive.
     * @param lo Lower bit index, inclusive.
     * @return Extracted field, right-aligned in a 16-bit word.
     *
     * Example: bits(0b1010110, 5, 3) returns 0b101.
     */
    inline std_word_t bits(std_word_t src, uint8_t hi, uint8_t lo) {
        std_word_t width = (std_word_t)(hi - lo + 1);
        std_word_t mask  = (width >= 16) ? 0xFFFF : (std_word_t)((1u << width) - 1);
        return (src >> lo) & mask;
    }

    /**
     * @brief Read a single bit from a 16-bit word.
     *
     * @param src Source word.
     * @param loc Bit position to read.
     * @return Bit value as 0 or 1.
     */
    inline uint8_t bit(std_word_t src, uint8_t loc) {
        return (src >> loc) & 0x1;
    }

    /**
     * @brief Sign-extend a value from the given bit width.
     *
     * @param num Value to extend.
     * @param bitc Number of bits in the original value.
     * @return Sign-extended 16-bit value.
     */
    inline std_word_t sign_extend(std_word_t num, uint8_t bitc) {
        if (bit(num, bitc - 1)) {
            num |= (0xFFFF << bitc);
        }
        return num;
    }
} // namespace lc3kit

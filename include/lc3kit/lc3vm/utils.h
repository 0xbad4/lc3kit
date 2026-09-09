#pragma once

#include <cstdint>
#include "enums.h"


namespace lc3kit::vm {
    /**
     * @brief Return a human-readable description for a VM error code.
     *
     * @param err Error value to translate.
     * @return Static string describing the error condition.
     */
    inline constexpr const char* err_str(error_type err) {
        switch (err) {
            case error_type::NO_ERROR:
                return "No error.";

            case error_type::MEM_ALLOC_FAIL:
                return "Memory allocation failed.";

            case error_type::INVALID_MEMORY:
                return "Invalid memory.";

            case error_type::INVALID_REGISTERS:
                return "Invalid register state.";

            case error_type::ILLEGAL_OPCODE:
                return "Illegal opcode.";

            case error_type::WRONG_EXEC_POLICY:
                return "Wrong execution policy set";

            case error_type::NO_HARDWARE:
                return "No hardware attached.";

            case error_type::HW_NO_KEYBOARD:
                return "Keyboard hardware is unavailable.";

            case error_type::HW_NO_DISPLAY:
                return "Display hardware is unavailable.";

            case error_type::PRIVILEGE_VIOLATION:
                return "Privilege violation.";

            case error_type::ILLEGAL_STATE:
                return "Illegal virtual machine state.";

            case error_type::DIVISION_BY_ZERO:
                return "Division by zero.";

            case error_type::OUT_OF_INSTRUCTIONS:
                return "No more instructions to execute.";
        }

        return "Unknown error.";
    }

} // namespace lc3kit

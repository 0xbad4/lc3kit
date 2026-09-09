#pragma once

#include "lc3kit/common.h"
#include <cstdint>

namespace lc3kit::vm
{  
    /**
     * @brief Execution policy / run mode.
     */
    enum class exec_policy : uint8_t {
        RUN,
        STEP
    };

    /**
     * Controls how the VM handles trap and interrupt service routines.
     *
     * BUILTIN_OS:
     *   Trap and interrupt service routines are implemented in C++.
     *   Memory and registers are still updated through the normal bus
     *   (mem_write, register_write, ssp_push/pop) so observers (debuggers,
     *   memory panels, register watches) see all state changes exactly as
     *   they would if a real lc3os image were running -- PSR switches,
     *   supervisor stack pushes/pops, R0 updates, KBSR/KBDR clears are
     *   all visible in real time.
     *
     * CUSTOM_OS:
     *   The VM is a pure hardware simulator. Trap vectors, interrupt vectors,
     *   and supervisor space are entirely owned by the user's loaded OS image.
     *   No C++ service routines fire -- all trap and interrupt handling runs
     *   as real LC-3 assembly code, with the VM staying completely out of the way.
    */
    enum class boot_mode : uint8_t {
        BUILTIN_OS,   // use C++ trap/interrupt implementations (default)
        CUSTOM_OS     // hands-off: user loads their own OS, VM does nothing
    };

    /**
     * @brief errors.
     */
    enum class error_type {
        NO_ERROR,
        MEM_ALLOC_FAIL,
        INVALID_MEMORY,
        INVALID_REGISTERS,
        ILLEGAL_OPCODE,
        WRONG_EXEC_POLICY,
        ALREADY_RUNNING,
        NO_HARDWARE,
        HW_NO_KEYBOARD,
        HW_NO_DISPLAY,
        PRIVILEGE_VIOLATION,
        ILLEGAL_STATE,
        DIVISION_BY_ZERO,
        OUT_OF_INSTRUCTIONS
    };
}

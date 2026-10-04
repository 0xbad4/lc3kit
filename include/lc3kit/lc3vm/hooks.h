#pragma once

#include <cstdint>
#include <functional>
#include "arch.h"

namespace lc3kit::vm
{
    
    class VM;

    /**
     * @brief Callback container for VM lifecycle, execution, memory, register,
     * trap, interrupt, and error events.
     *
     * The structure is used to attach observer hooks that run before or after
     * key VM actions without changing the core execution flow.
     * 
     * @note `on_breakpoint` returns a @c bool: @c true means keep running, @c false means pause. 
     *        There is no separate @c resume(). To continue after a pause, call @c run() again, 
     *        it detects the paused-on-breakpoint state and continues from there.
     */
    struct vm_hooks {
        
        // life cycle
        std::function<void(VM&)> on_start = nullptr;
        std::function<void(VM&)> on_reset = nullptr;
        
        // ----------- Execution
        
        // params: VM ref, PC
        std::function<void(VM&, std_word_t pc)> before_instruction = nullptr;   
        std::function<void(VM&, std_word_t pc)> after_instruction = nullptr;
        
        // ----------- Memory
        
        // params: VM ref, addr
        std::function<void(VM&, std_word_t addr)> before_memory_read = nullptr;
        
        // params: VM ref, addr, value
        std::function<void(VM&, std_word_t addr, std_word_t value)> after_memory_read = nullptr;

        // params: VM ref, addr, old_value, new_value
        std::function<void(VM&, std_word_t addr, std_word_t oldv, std_word_t newv)> before_memory_write = nullptr;

        // params: VM ref, addr, value
        std::function<void(VM&, std_word_t addr, std_word_t value)> after_memory_write = nullptr;

        
        // ----------- Registers

        // params: VM ref, register, old_value, new_value  
        std::function<void(VM&, registers reg, std_word_t oldv, std_word_t newv)> before_register_write = nullptr;

        // params: VM ref, register, value
        std::function<void(VM&, registers reg, std_word_t value)> after_register_write = nullptr;

        
        // ----------- Breakpoints

        // params: VM ref, addr
        std::function<bool(VM&, std_word_t addr)> on_breakpoint = nullptr;

        
        // ----------- TRAP
        
        // params: VM ref, vector
        std::function<void(VM&, uint8_t tvector)> before_trap = nullptr;
        std::function<void(VM&, uint8_t tvector)> after_trap = nullptr;
        
        // ----------- Interrupts

        // params: VM ref, interrupt
        std::function<void(VM&, interrupts intr)> before_interrupt = nullptr;
        std::function<void(VM&, interrupts intr)> after_interrupt = nullptr;

        
        // ----------- Errors

        // params: VM ref, instruction
        std::function<void(VM&, std_word_t instr)> on_invalid_instruction = nullptr;

        // params: VM ref, addr, is_write
        std::function<void(VM&, std_word_t addr, bool is_write)> on_invalid_memory_access = nullptr;

        // params: VM ref, PC, Forced
        std::function<void(VM&, std_word_t pc, bool forced)> on_halt = nullptr;
    };
} // namespace lc3kit

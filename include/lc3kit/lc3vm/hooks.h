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
     */
    struct vm_hooks {
        
        // life cycle
        std::function<void(VM&)> on_start = nullptr;
        std::function<void(VM&)> on_reset = nullptr;
        
        // ----------- Execution
        
        // params: VM ref, PC
        std::function<void(VM&, std_word_t)> before_instruction = nullptr;   
        std::function<void(VM&, std_word_t)> after_instruction = nullptr;
        
        // ----------- Memory
        
        // params: VM ref, addr
        std::function<void(VM&, std_word_t)> before_memory_read = nullptr;
        
        // params: VM ref, addr, value
        std::function<void(VM&, std_word_t, std_word_t)> after_memory_read = nullptr;

        // params: VM ref, addr, old_value, new_value
        std::function<void(VM&, std_word_t, std_word_t, std_word_t)> before_memory_write = nullptr;

        // params: VM ref, addr, value
        std::function<void(VM&, std_word_t, std_word_t)> after_memory_write = nullptr;

        
        // ----------- Registers

        // params: VM ref, register, old_value, new_value  
        std::function<void(VM&, registers, std_word_t, std_word_t)> before_register_write = nullptr;

        // params: VM ref, register, value
        std::function<void(VM&, registers, std_word_t)> after_register_write = nullptr;

        
        // ----------- Breakpoints

        // params: VM ref, addr
        std::function<bool(VM&, std_word_t)> on_breakpoint = nullptr;

        
        // ----------- TRAP
        
        // params: VM ref, vector
        std::function<void(VM&, uint8_t)> before_trap = nullptr;
        std::function<void(VM&, uint8_t)> after_trap = nullptr;
        
        // ----------- Interrupts

        // params: VM ref, interrupt
        std::function<void(VM&, interrupts)> before_interrupt = nullptr;
        std::function<void(VM&, interrupts)> after_interrupt = nullptr;

        
        // ----------- Errors

        // params: VM ref, instruction
        std::function<void(VM&, std_word_t)> on_invalid_instruction = nullptr;

        // params: VM ref, addr, is_write
        std::function<void(VM&, std_word_t, bool)> on_invalid_memory_access = nullptr;

        // params: VM ref, PC, Forced
        std::function<void(VM&, std_word_t, bool)> on_halt = nullptr;
    };
} // namespace lc3kit

#pragma once

#include <fstream>
#include <iostream>
#include <cstdio>
#include <cstring>

#include "arch.h"
#include "utils.h"
#include "enums.h"
#include "hooks.h"
#include "hw.h"

#include "lc3kit/common.h"

namespace lc3kit::vm
{
    // little shortcut
    #define self (*this)

    class VM {
        protected:
            exec_policy   m_expo;
            boot_mode     m_boot_mode;
            error_type    m_last_error = error_type::NO_ERROR;
            bool             m_running = false;
            bool             m_explicit_stop = false;
            bool             m_ext_enable = false;
            bool             m_paused_on_breakpoint = false;
            std_word_t    m_origin = 0;
            sections_addr m_sections;

            vm_keyboard *m_keyboard = nullptr;
            vm_display  *m_display = nullptr;
            vm_hooks     m_hooks;

            std_word_t      m_current_instruction = 0;
            std_word_t      m_saved_ssp = 0;
            std_word_t      m_saved_usp = 0;

            breakpoints_t m_breakpoints;

            // memory
            std_word_t *m_memory = nullptr;
            std_word_t  m_used_memory_size = 0;

            // registers
            std_word_t *m_registers = nullptr;
            
            // ----------------- Setup
            bool reserve_memory() {
                // if not valid, reserve it
                if (!is_memory_valid()) {
                    m_memory = new (std::nothrow) std_word_t[MEMORY_MAX]();
                    return m_memory != nullptr;
                }
                // if already reserved, just zero it
                else {
                    std::memset(m_memory, 0, MEMORY_MAX * sizeof(std_word_t));
                }

                return true;
            }

            bool reserve_registers() {
                if (!m_registers) {
                    m_registers = new (std::nothrow) std_word_t[(uint8_t)registers::_R_COUNT]();
                    return m_registers != nullptr;
                }

                else {
                    std::memset(m_registers, 0, (uint8_t)registers::_R_COUNT * sizeof(std_word_t));
                }

                return true;
            }

            // ----------------- Execution
            void fetch_and_exec() {
                std_word_t pc_ = pc();

                // breakpoints check
                // NOTE: stop execution, unless a hook said otherwise (return true)
                if (m_breakpoints.test(pc_)) {
                    m_running = false;

                    if (m_hooks.on_breakpoint) {
                        if (!m_hooks.on_breakpoint(self, pc_)) {
                            m_paused_on_breakpoint = true;
                            return;
                        }
                    }
                    else {
                        m_paused_on_breakpoint = true;
                        return;
                    }
                }

                // NOTE: instructions require incremented PC
                pc(true);  // increment 

                m_current_instruction = mem_read(pc_);

                // opcode is represented by the 4 MSBs [16, 15, 14, 13]
                uint8_t  opcode = m_current_instruction >> 12;

                if (m_hooks.before_instruction)
                    m_hooks.before_instruction(self, pc_);

                switch ((opcodes)opcode) {
                    case opcodes::BR:
                        br();
                        break;

                    case opcodes::ADD:
                        add();
                        break;

                    case opcodes::LD:
                        ld();
                        break;

                    case opcodes::ST:
                        st();
                        break;

                    case opcodes::JSR:
                        jsr();
                        break;

                    case opcodes::AND:
                        and_();
                        break;

                    case opcodes::LDR:
                        ldr();
                        break;

                    case opcodes::STR:
                        str();
                        break;

                    case opcodes::RTI:
                        rti();
                        break;

                    case opcodes::NOT:
                        not_();
                        break;

                    case opcodes::LDI:
                        ldi();
                        break;

                    case opcodes::STI:
                        sti();
                        break;

                    case opcodes::JMP:
                        jmp();
                        break;

                    case opcodes::EXT:
                        ext();
                        break;

                    case opcodes::LEA:
                        lea();
                        break;

                    case opcodes::TRAP:
                        trap();
                        break;

                    default:
                        set_error(error_type::ILLEGAL_OPCODE);
                        m_running = false;
                        break;
                }

                if (m_hooks.after_instruction) m_hooks.after_instruction(self, pc());
            }
            
            // ----------------- Instructions
            void br() {
                /* (branch)
                    The condition codes specified by the state of bits [11:9] are tested. If bit [11] is
                    set, N is tested; if bit [11] is clear, N is not tested. If bit [10] is set, Z is tested, etc.
                    If any of the condition codes tested is set, the program branches to the location
                    specified by adding the sign-extended PCoffset9 field to the incremented PC.
                */
                // 15         12 11     9 8               0
                // |  opcode    |  n/z/p |     PCoffset9  |

                std_word_t cond_mask = bits(m_current_instruction, 11, 9);
                std_word_t offset = sign_extend(bits(m_current_instruction, 8, 0), 9);

                std_word_t current_cond = reg_read(registers::PSR) & 0x0007;

                if (cond_mask & current_cond) {
                    self[registers::PC] = pc() + offset;
                }
            }

            void add() {
                /*
                    If bit [5] is 0, the second source operand is obtained from SR2. If bit [5] is 1, the
                    second source operand is obtained by sign-extending the imm5 field to 16 bits.
                    In both cases, the second source operand is added to the contents of SR1 and the
                    result stored in DR. The condition codes are set, based on whether the result is
                    negative, zero, or positive.
                */
                // 15         12 11     9 8      6   5   4              0
                // |  opcode    |   DR   |  SR1  |  mode | imm5 /  SR2  |
                // destination register
                registers dr  = (registers)bits(m_current_instruction, 11, 9);

                // source register 1
                registers sr1 = (registers)bits(m_current_instruction, 8, 6);

                // immediate mode
                bool imm_mode = bit(m_current_instruction, 5);

                std_word_t sr1_val = reg_read(sr1);
                std_word_t result;

                // ADD R0, R1, 9
                if (imm_mode) {
                    std_word_t imm5 = sign_extend(bits(m_current_instruction, 4, 0), 5);
                    result = sr1_val + imm5;

                } 
                // ADD, R0, R1, R3
                else {
                    registers sr2 = (registers)bits(m_current_instruction, 2, 0);
                    result = sr1_val + reg_read(sr2);
                }

                reg_write(dr, result);
            }

            void ld() {
                /* (load)
                    An address is computed by sign-extending bits [8:0] to 16 bits and adding this
                    value to the incremented PC. The contents of memory at this address are loaded
                    into DR. The condition codes are set, based on whether the value loaded is
                    negative, zero, or positive.
                */
                // 15         12 11     9 8               0
                // |  opcode    |   DR   |     PCoffset9  |

                registers dr = (registers)bits(m_current_instruction, 11, 9);
                std_word_t offset = sign_extend(bits(m_current_instruction, 8, 0), 9);
                std_word_t addr = pc() + offset;

                std_word_t value = mem_read(addr);
                reg_write(dr, value);
            }

            void st() {
                /* (store)
                    The contents of the register specified by SR are stored in the memory location
                    whose address is computed by sign-extending bits [8:0] to 16 bits and adding this
                    value to the incremented PC.
                */
                // 15         12 11     9 8               0
                // |  opcode    |   SR   |     PCoffset9  |
                registers sr = (registers)bits(m_current_instruction, 11, 9);
                uint16_t offset = sign_extend(bits(m_current_instruction, 8, 0), 9);
                uint16_t addr = pc() + offset;
                uint16_t value = reg_read(sr);

                mem_write(addr, value);
            }

            void jsr() {
                /* (jump to subroutine)
                    First, the incremented PC is saved in R7. This is the linkage back to the calling
                    routine. Then the PC is loaded with the address of the first instruction of the
                    subroutine, causing an unconditional jump to that address. The address of the
                    subroutine is obtained from the base register (if bit [11] is 0), or the address is
                    computed by sign-extending bits [10:0] and adding this value to the incremented
                    PC (if bit [11] is 1).
                */
                // 15         12 11     9 8      6 B  5              0
                // |  opcode    |  1/0   | BaseR  | 000 | PCoffset11 |

                bool use_offset_mode = bit(m_current_instruction, 11);

                std_word_t return_addr = pc();

                // NOTE: JSR/JSRR does NOT set condition codes. Write R7 directly (still firing the
                // register hooks) rather than going through reg_write(), which
                // would incorrectly touch N/Z/P as a side effect.
                reg_write(registers::R7, return_addr);

                if (use_offset_mode) {
                    // JSR: PC = PC + SEXT(PCoffset11)
                    std_word_t offset = sign_extend(bits(m_current_instruction, 10, 0), 11);
                    self[registers::PC] = return_addr + offset;
                } else {
                    // JSRR: PC = BaseR
                    registers baseR = (registers)bits(m_current_instruction, 8, 6);
                    self[registers::PC] = reg_read(baseR);
                }
            }

            void and_() {
                /*
                    If bit [5] is 0, the second source operand is obtained from SR2. If bit [5] is 1,
                    the second source operand is obtained by sign-extending the imm5 field to 16
                    bits. In either case, the second source operand and the contents of SR1 are bit-
                    wise ANDed, and the result stored in DR. The condition codes are set, based on
                    whether the binary value produced, taken as a 2’s complement integer, is negative,
                    zero, or positive.
                */
                // 15         12 11     9 8      6   5   4              0
                // |  opcode    |   DR   |  SR1  |  mode | imm5 /  SR2  |

                registers dr  = (registers)bits(m_current_instruction, 11, 9);
                registers sr1 = (registers)bits(m_current_instruction, 8, 6);

                bool imm_mode = bit(m_current_instruction, 5);

                std_word_t sr1_val = reg_read(sr1);
                std_word_t result;

                if (imm_mode) {
                    std_word_t imm5 = sign_extend(bits(m_current_instruction, 4, 0), 5);
                    result = sr1_val & imm5;
                } else {
                    registers sr2 = (registers)bits(m_current_instruction, 2, 0);
                    result = sr1_val & reg_read(sr2);
                }

                reg_write(dr, result);

            }

            void ldr() {
                /* (load base + offset)
                    An address is computed by sign-extending bits [5:0] to 16 bits and adding this
                    value to the contents of the register specified by bits [8:6]. The contents of memory
                    at this address are loaded into DR. The condition codes are set, based on whether
                    the value loaded is negative, zero, or positive.
                */
                // 15         12 11     9 8      6   5              0
                // |  opcode    |   DR   | BaseR |     offset6    |
                registers dr    = (registers)bits(m_current_instruction, 11, 9);
                registers baseR = (registers)bits(m_current_instruction, 8, 6);
                std_word_t offset = sign_extend(bits(m_current_instruction, 5, 0), 6);
                std_word_t addr = reg_read(baseR) + offset;

                std_word_t value = mem_read(addr);

                reg_write(dr, value);
            }

            void str() {
                /* (store base + offset)
                    The contents of the register specified by SR are stored in the memory location
                    whose address is computed by sign-extending bits [5:0] to 16 bits and adding this
                    value to the contents of the register specified by bits [8:6].
                */
                // 15         12 11     9 8      6   5              0
                // |  opcode    |   SR   | BaseR |     offset6    |
                registers sr    = (registers)bits(m_current_instruction, 11, 9);
                registers baseR = (registers)bits(m_current_instruction, 8, 6);
                std_word_t offset = sign_extend(bits(m_current_instruction, 5, 0), 6);
                std_word_t addr = reg_read(baseR) + offset;
                std_word_t value = reg_read(sr);

                mem_write(addr, value);
            }

            void rti() {
                /*
                    If the processor is running in Supervisor mode, the top two elements on the
                    Supervisor Stack are popped and loaded into PC, PSR. If the processor is running
                    in User mode, a privilege mode violation exception occurs.
                */
                // 15         12 11     9 8      6   5              0
                // |  opcode    |  000   | 00000 |     000000    |
                if (bit(reg_read(registers::PSR), 15)) {
                    set_error(error_type::ILLEGAL_OPCODE);
                    m_running = false;
                    return;
                }

                std_word_t restored_psr = ssp_pop();
                std_word_t restored_pc  = ssp_pop();
                self[registers::PSR] = restored_psr;
                self[registers::PC]  = restored_pc;

                if (bit(restored_psr, 15)) {
                    m_saved_ssp            = reg_read(registers::R6);
                    self[registers::R6] = m_saved_usp;
                }
            }

            void not_() {
                /*
                    The bit-wise complement of the contents of SR is stored in DR. The condi-
                    tion codes are set, based on whether the binary value produced, taken as a 2’s
                    complement integer, is negative, zero, or positive.
                */
                // 15         12 11     9 8      6   5              0
                // |  opcode    |   DR   |  SR   | 111111
                registers dr = (registers)bits(m_current_instruction, 11, 9);
                registers sr = (registers)bits(m_current_instruction, 8, 6);

                std_word_t result = ~reg_read(sr);

                reg_write(dr, result);
            }

            void ldi() {
                /*  (pointer like load)
                    An address is computed by sign-extending bits [8:0] to 16 bits and adding this
                    value to the incremented PC. What is stored in memory at this address is the
                    address of the data to be loaded into DR. The condition codes are set, based on
                    whether the value loaded is negative, zero, or positive.
                */
                // 15         12 11     9 8               0
                // |  opcode    |   DR   |     PCoffset9  |
                registers dr = (registers)bits(m_current_instruction, 11, 9);
                std_word_t offset = sign_extend(bits(m_current_instruction, 8, 0), 9);
                std_word_t addr1 = pc() + offset;

                std_word_t addr2 = mem_read(addr1);
                std_word_t value = mem_read(addr2);
                reg_write(dr, value);

            }

            void sti() {
                /* (store indirect)
                    The contents of the register specified by SR are stored in the memory location
                    whose address is obtained as follows: Bits [8:0] are sign-extended to 16 bits and
                    added to the incremented PC. What is in memory at this address is the address of
                    the location to which the data in SR is stored.
                */
                // 15         12 11     9 8               0
                // |  opcode    |   SR   |     PCoffset9  |
                registers sr = (registers)bits(m_current_instruction, 11, 9);
                std_word_t offset = sign_extend(bits(m_current_instruction, 8, 0), 9);
                std_word_t addr1 = pc() + offset;
                std_word_t addr2 = mem_read(addr1);
                std_word_t value = reg_read(sr);

                mem_write(addr2, value);
            }

            void jmp() {
                /*
                    The program unconditionally jumps to the location specified by the contents of
                    the base register. Bits [8:6] identify the base register.
                */
                // 15         12 11     9 8      6 B  5    0
                // |  opcode    |  000   | BaseR  | 000000 |
                registers baseR = (registers)bits(m_current_instruction, 8, 6);
                self[registers::PC] = reg_read(baseR);
                // NOTE: RET is just JMP R7 by convention -- no separate handling required,
                // this same code path covers it.

                /*
                JMP R2
                RET ; PC ← R2
                */
            }

            void lea() {
                /* (load effective address)
                    An address is computed by sign-extending bits [8:0] to 16 bits and adding this
                    value to the incremented PC. This address is loaded into DR.‡ The condition
                    codes are set, based on whether the value loaded is negative, zero, or positive.
                */
                // 15         12 11     9 8               0
                // |  opcode    |   DR   |     PCoffset9  |

                registers dr = (registers)bits(m_current_instruction, 11, 9);
                std_word_t offset = sign_extend(bits(m_current_instruction, 8, 0), 9);
                std_word_t addr = pc() + offset;

                reg_write(dr, addr);
            }

            void trap() {
                /*
                    First R7 is loaded with the incremented PC. (This enables a return to the instruction
                    physically following the TRAP instruction in the original program after the service
                    routine has completed execution.) Then the PC is loaded with the starting address
                    of the system call specified by trapvector8. The starting address is contained in
                    the memory location whose address is obtained by zero-extending trapvector8 to
                    16 bits.
                */
                // 15         12 11     9 8              0
                // |  opcode    |  000   |     trapvect8 |
                trap_vectors vector = (trap_vectors)bits(m_current_instruction, 7, 0);

                if (m_hooks.before_trap) m_hooks.before_trap(self, (uint8_t)vector);

                // mirror exactly what lc3os does on trap entry

                std_word_t old_psr = reg_read(registers::PSR);
                std_word_t old_pc  = pc();  // already incremented by fetch

                // 1. switch to supervisor mode (PSR[15] = 0)
                std_word_t new_psr = old_psr & ~(1 << 15);
                self[registers::PSR] = new_psr;

                // 2. if we were in user mode, swap R6 to supervisor stack pointer
                if (bit(old_psr, 15)) {
                    m_saved_usp          = reg_read(registers::R6);
                    self[registers::R6] = m_saved_ssp;
                }

                // 3. push PC then PSR onto supervisor stack (PC first, PSR on top)
                ssp_push(old_pc);
                ssp_push(old_psr);

                // implementations using memory as the bus
                if (m_boot_mode == boot_mode::BUILTIN_OS) {
                    switch (vector) {
                        case trap_vectors::GETC: {
                            get_char();
                            break;
                        }
                        case trap_vectors::OUT: {
                            put(reg_read(registers::R0));
                            break;
                        }
                        case trap_vectors::PUTS: {
                            puts(reg_read(registers::R0));
                            break;
                        }
                        case trap_vectors::IN: {
                            get_char(true);
                            break;
                        }
                        case trap_vectors::PUTSP: {
                            puts(reg_read(registers::R0), true);
                            break;
                        }
                        case trap_vectors::HALT: {
                            halt();
                            break;
                        }
                        default:
                            set_error(error_type::ILLEGAL_OPCODE);
                            m_running = false;
                            if (m_hooks.on_invalid_instruction)
                                m_hooks.on_invalid_instruction(self, m_current_instruction);
                            break;
                    }

                    if (m_running) {
                        rti();
                    }

                }
                
                else {
                    // jump to user's trap service routine via their vector table
                    (self)[registers::PC] = mem_read((std_word_t)vector);
                }

                if (m_hooks.after_trap) m_hooks.after_trap(self, (uint8_t)vector);
            }

            void ext() {
                // If extension not enabled
                if (!m_ext_enable) {
                    set_error(error_type::ILLEGAL_OPCODE);
                    m_running = false;

                    if (m_hooks.on_invalid_instruction)
                        m_hooks.on_invalid_instruction(self, m_current_instruction);

                    return;
                }

                ext_opcodes sub_op  = (ext_opcodes)bits(m_current_instruction, 11, 10);
                registers dr     = (registers)bits(m_current_instruction, 9, 7);
                registers sr1    = (registers)bits(m_current_instruction, 6, 4);
                bool reg_mode    = bit(m_current_instruction, 3);

                std_word_t sr1_val = reg_read(sr1);
                std_word_t operand;

                if (reg_mode) {
                    registers sr2 = (registers)bits(m_current_instruction, 2, 0);
                    operand = reg_read(sr2);
                } else {
                    operand = sign_extend(bits(m_current_instruction, 2, 0), 3);
                }

                switch (sub_op) {
                    case ext_opcodes::SHL: ext_shl(dr, sr1_val, operand); break;
                    case ext_opcodes::SHR: ext_shr(dr, sr1_val, operand); break;
                    case ext_opcodes::MUL: ext_mul(dr, sr1_val, operand); break;
                    case ext_opcodes::DIV: ext_div(dr, sr1_val, operand); break;
                }
            }

            void ext_shl(registers dr, std_word_t sr1_val, std_word_t operand) {
                reg_write(dr, sr1_val << operand);
            }

            void ext_shr(registers dr, std_word_t sr1_val, std_word_t operand) {
                reg_write(dr, (std_word_t)((int16_t)sr1_val >> operand));
            }

            void ext_mul(registers dr, std_word_t sr1_val, std_word_t operand) {
                reg_write(dr, sr1_val * operand);
            }

            void ext_div(registers dr, std_word_t sr1_val, std_word_t operand) {
                if (operand == 0) {
                    set_error(error_type::DIVISION_BY_ZERO);
                    m_running = false;
                    if (m_hooks.on_invalid_instruction) {
                        m_hooks.on_invalid_instruction(self, m_current_instruction);
                    }
                    return;
                }
                reg_write(dr, sr1_val / operand);
            }

            // ----------------- Supervisor Stack
            void ssp_push(std_word_t value) {
                std_word_t ssp = reg_read(registers::R6);
                self[--ssp] = value;
                self[registers::R6] = ssp;
            }

            std_word_t ssp_pop() {
                std_word_t ssp   = reg_read(registers::R6);
                std_word_t value = mem_read(ssp++);
                self[registers::R6] = ssp;
                return value;
            }

            // ----------------- Interrupts
            void service_interrupt() {
                // keyboard interrupt conditions:
                // 1. KBSR[15] = 1 (character ready)
                // 2. KBSR[14] = 1 (interrupt enabled)
                // 3. keyboard priority (PL4 = 0b100) > current PSR priority
                std_word_t kbsr     = m_memory[(std_word_t)mmio::KBSR];
                bool char_ready   = bit(kbsr, 15);
                bool int_enabled  = bit(kbsr, 14);
                uint8_t current_priority = (uint8_t)bits(reg_read(registers::PSR), 10, 8); // PSR[8:10]

                if (!char_ready || !int_enabled || (4 <= current_priority)) { 
                    return;
                }

                // NOTE: lc3 has only one interrupt
                if(m_hooks.before_interrupt) m_hooks.before_interrupt(self, interrupts::KEYBOARD);

                // save context BEFORE mutating PSR/PC -- this is exactly what gets
                // restored on RTI, so it must be captured before we touch either.
                std_word_t old_pc  = pc();
                std_word_t old_psr = reg_read(registers::PSR);

                // enter supervisor mode (PSR[15] = 0)
                std_word_t new_psr = old_psr & ~(1 << 15);

                // clear current priority bits [10:8]
                new_psr &= ~(0x7 << 8);

                // set priority to PL4 -- keyboard interrupt runs at priority 4
                new_psr |= (0x4 << 8);

                (self)[registers::PSR] = new_psr;

                // if we were in user mode, swap R6 from USP to SSP
                // R6 always holds the active stack pointer for the current privilege level
                if (bit(old_psr, 15)) {
                    m_saved_usp            = reg_read(registers::R6); // save user stack
                    (self)[registers::R6] = m_saved_ssp;             // load supervisor stack
                }

                // push the ORIGINAL pc/psr, same for both boot modes -- mirrors trap()
                ssp_push(old_pc);
                ssp_push(old_psr);

                if (m_boot_mode == boot_mode::BUILTIN_OS) {
                    int_keyboard();
                }
                else {
                    // assuming an OS that can handle interrupts is loaded
                    (self)[registers::PC] = mem_read(INTERRUPT_VEC_BASE_ADDR + (uint8_t)interrupts::KEYBOARD);
                }

                if(m_hooks.after_interrupt)
                    m_hooks.after_interrupt(self, interrupts::KEYBOARD);
            }

            // BUILT-IN keyboard interrupt subroutine 
            void int_keyboard() {
                // pc/psr already saved and pushed by service_interrupt() -- don't redo it here
                (self)[registers::PC] = mem_read(INTERRUPT_VEC_BASE_ADDR + (uint8_t)interrupts::KEYBOARD);

                // read character from KBDR -- consume and clear, same as get_char()
                std_word_t ch = get_kbdr();

                // store into R0, same as lc3os keyboard ISR does
                reg_write(registers::R0, ch);

                // pop pc/psr pushed by service_interrupt() and resume
                rti();
            }

            // ----------------- BUILT-IN Traps
            void halt() {
                // NOT FORCED
                stop_(false);
            }

            void get_char(bool echo=false) {
                if (!m_keyboard) {
                    set_error(error_type::HW_NO_KEYBOARD);
                    return;
                }

                if (!m_keyboard->is_memory_valid()) {
                    m_keyboard->set_addr(&m_memory[(std_word_t)mmio::KBSR], &m_memory[(std_word_t)mmio::KBDR]);
                }

                // flush any pending keyboard input so a key pressed before waiting
                // isn't mistakenly treated as the user's response to the prompt.
                get_kbdr(true);  // flush leftover

                // wait until keyboard hardware says a key is available.
                while (m_running && !bit(m_memory[(std_word_t)mmio::KBSR], 15)) {
                    // NOTE: busy wait.
                    //       the keyboard device (possibly another thread) will eventually - 
                    //       set KBSR[15] and write the character into KBDR.
                }

                std_word_t ch = get_kbdr();
                reg_write(registers::R0, ch);

                // if interrupt is IN, echo is enabled
                if (echo) {
                    put(ch);
                }
            }

            void put(std_word_t ch) {
                if (!m_display) {
                    set_error(error_type::HW_NO_DISPLAY);
                    return;
                }

                if (!m_display->is_memory_valid()) {
                    m_display->set_addr(&m_memory[(std_word_t)mmio::DSR], &m_memory[(std_word_t)mmio::DDR]);
                }

                // wait until the display is ready.
                while (m_running && !bit(m_memory[(std_word_t)mmio::DSR], 15)){
                    // same as getc
                }

                // CPU writes to DDR.
                mem_write((std_word_t)mmio::DDR, ch & 0xFF);
            }

            void puts(std_word_t addr, bool packed=false) {
                std_word_t ch = mem_read(addr);

                while (m_running && ch != 0) {
                    // lower by default in both modes
                    
                    put(ch);

                    // if packed, each ch contains two chars
                    if (packed) {
                        // upper
                        uint8_t upper = ch >> 8;
                        
                        if (upper) { put(upper); }
                    }

                    ch = mem_read(++addr);
                }
            }

            // ----------------- Utils
            void set_error(error_type err) {
                m_last_error = err;
            }

            void reg_write(registers reg, std_word_t value) {
                // PSR is only writable in supervisor mode (PSR[15] == 0)
                // a user-mode program attempting to write PSR is a privilege violation
                if (reg == registers::PSR) {
                    if (bit(reg_read(registers::PSR), 15)) {
                        set_error(error_type::PRIVILEGE_VIOLATION);
                        m_running = false;
                        if (m_hooks.on_invalid_instruction)
                            m_hooks.on_invalid_instruction(self, m_current_instruction);
                        return;
                    }
                }

                std_word_t old_value = reg_read(reg);

                if (m_hooks.before_register_write)
                    m_hooks.before_register_write(self, reg, old_value, value);

                self[reg] = value;

                if (m_hooks.after_register_write)
                    m_hooks.after_register_write(self, reg, value);

                update_cond(value);
            }

            std_word_t get_kbdr(bool destroy=true) {
                std_word_t c = m_memory[(std_word_t)mmio::KBDR];
                // this one is destroyed any way to prevent collision between GETC and Interrupt
                m_memory[(std_word_t)mmio::KBSR] &= ~(1u << 15);

                if (destroy) {
                    m_memory[(std_word_t)mmio::KBDR]  = 0;
                }

                return c;
            }

            std_word_t pc(bool inc) {
                std_word_t value = reg_read(registers::PC);
                if (inc) self[registers::PC] += 1;
                return value;
            }

             // change program counter value
            void set_pc(std_word_t value) {
                self[registers::PC] = value;
            }

            // update condition flags based on a specific result value
            // (used internally by instruction handlers right after they
            // compute/load a value into a register)
            // for debugging purposes, this is public so that the user can manually set the flags
            void update_cond(std_word_t result) {
                std_word_t flags;
                
                if (result == 0) {
                    flags = (std_word_t)r_cond::ZRO;
                } else if (bit(result, 15)) {
                    // bit 15 set -> negative in two's complement
                    flags = (std_word_t)r_cond::NEG;
                } else {
                    flags = (std_word_t)r_cond::POS;
                }

                // PSR bits [2:0] hold N/Z/P; clear them, then OR in exactly one
                std_word_t psr_val = reg_read(registers::PSR);
                psr_val &= ~0x0007;
                psr_val |= flags;
                self[registers::PSR] = psr_val;
            }

            std_word_t& operator[](std_word_t mem_addr) {
                return m_memory[mem_addr];
            }

            std_word_t& operator[](registers reg) {
                return m_registers[(uint8_t)reg];
            }

            bool is_in_bound() {
                for (const auto& sec : m_sections) {
                    if (pc() >= sec.origin && pc() < (std_word_t)(sec.origin + sec.size)) {
                        return true;
                    }
                }

                return false;
            }

            bool handle_after_breakpoint() {
                // if resuming from a breakpoint, skip on_start and execute the
                // paused instruction first before re-entering the loop normally
                if (m_paused_on_breakpoint) {
                    m_paused_on_breakpoint = false;
                    std_word_t pc_ = pc();
                    m_breakpoints.reset(pc_);  // prevent re-triggering on resume
                    fetch_and_exec();          // execute the instruction we paused on
                    m_breakpoints.set(pc_);    // restore

                    return true;
                }

                return false;
            }

            error_type run_() {
                while (m_running) {
                    // stop if PC falls outside all loaded sections
                    if (!is_in_bound()) {
                        break;
                    }

                    // check for interrupts
                    service_interrupt();

                    // fetch and exec
                    fetch_and_exec();
                }

                // execution has stopped but not due to a breakpoint or halt instruction -- notify observer
                if (!m_paused_on_breakpoint && !m_explicit_stop) {
                    stop_(true);
                }

                return m_last_error;
            }

            void stop_(bool f) {
                m_running = false;
                m_explicit_stop = true;
                
                if (m_hooks.on_halt) {
                    m_hooks.on_halt(self, pc(), f);
                }
            }

        public:
            /**
             * @brief Construct a VM instance with execution and boot configuration.
             *
             * @param expo Execution policy for the VM.
             * @param bm Boot mode used for trap/interrupt handling.
             */
            VM(exec_policy expo = exec_policy::RUN, boot_mode bm = boot_mode::BUILTIN_OS)
                : m_expo(expo), m_boot_mode(bm) { }

            VM(const VM&)            = delete;
            VM& operator=(const VM&) = delete;

            // ----------------- Setters
            /**
             * @brief Attach the keyboard hardware device to the VM.
             *
             * @param keyboard Keyboard interface instance.
             */
            void set_hw_keyboard(vm_keyboard *keyboard) {
                if (m_running) {
                    set_error(error_type::ILLEGAL_STATE);
                    return;
                }
                if (!keyboard) {
                    set_error(error_type::HW_NO_KEYBOARD);
                    return;
                }
                m_keyboard = keyboard;
            }

            /**
             * @brief Attach the display hardware device to the VM.
             *
             * @param display Display interface instance.
             */
            void set_hw_display(vm_display *display) {
                if (m_running) {
                    set_error(error_type::ILLEGAL_STATE);
                    return;
                }
                if (!display) {
                    set_error(error_type::HW_NO_DISPLAY);
                    return;
                }
                m_display = display;
            }

            /**
             * @brief Write a value to VM memory at the given address.
             *
             * @param addr Address to write.
             * @param value Value to store.
             * @param hk If true, dispatch any configured memory hooks.
             */
            void mem_write(std_word_t addr, std_word_t value, bool hk=true) {
                // check if memory is allocated and valid
                if (!m_memory) {
                    set_error(error_type::INVALID_MEMORY);
                    return;
                }

                // enforce privilege level memory access restrictions
                // user mode (PSR[15] = 1) cannot access supervisor memory or device registers
                if (bit(reg_read(registers::PSR), 15)) {
                    // Check if writing to supervisor space (addresses below 0x3000)
                    bool is_supervisor_space = addr < 0x3000;
                    
                    // Check if writing to memory-mapped I/O device registers
                    bool is_device_registers = addr >= (std_word_t)mmio::KBSR;

                    // If attempting to write to restricted areas, trigger privilege violation
                    if (is_supervisor_space || is_device_registers) {
                        set_error(error_type::PRIVILEGE_VIOLATION);
                        m_running = false;
                        
                        // Notify observer of invalid memory access attempt
                        if (m_hooks.on_invalid_memory_access)
                            m_hooks.on_invalid_memory_access(self, addr, true);
                        return;
                    }
                }

                std_word_t old_value = mem_read(addr, false);

                if (m_hooks.before_memory_write && hk)
                    m_hooks.before_memory_write(self, addr, old_value, value);

                m_memory[addr] = value;

                // MMIO side effects
                if (addr == (std_word_t)mmio::DDR && m_display) {
                    m_memory[(std_word_t)mmio::DSR] |= (1 << 15);
                    m_display->on_write();
                }

                if (addr == (std_word_t)mmio::MCR) {
                    if (!bit(value, 15)) {
                        halt();
                        return;
                    }
                }

                if (m_hooks.after_memory_write && hk)
                    m_hooks.after_memory_write(self, addr, value);
            }
            
            /**
             * @brief Enable or disable the lc3kit extension opcode set.
             *
             * @param enabled True to enable extension instructions.
             */
            void set_ext_enabled(bool enabled) {
                if (m_running) {
                    set_error(error_type::ILLEGAL_STATE);
                    return;
                }
                m_ext_enable = enabled;
            }

            /**
             * @brief Set the VM execution policy.
             *
             * @param policy New execution policy value.
             */
            void set_exec_policy(exec_policy policy) {
                if (m_running) {
                    set_error(error_type::ILLEGAL_STATE);
                    return;
                }
                m_expo = policy;
            }
            
            /**
             * @brief Set the boot mode used for built-in traps and interrupts.
             *
             * @param mode Boot mode selection.
             */
            void set_boot_mode(boot_mode mode) {
                if (m_running) {
                    set_error(error_type::ILLEGAL_STATE);
                    return;
                }
                m_boot_mode = mode;
            }
            
            /**
             * @brief Set the load origin and program counter.
             *
             * Not allowed while VM is running unless its `STEP` mode.
             * 
             * @param o New origin address.
             */
            void set_origin(std_word_t o) {
                if (m_running || is_debug()) {
                    set_error(error_type::ILLEGAL_STATE);
                    return;
                }
                m_origin = o;
                set_pc(m_origin);
            }

            // ----------------- Getters
            /**
             * @brief Check whether the VM is currently running.
             *
             * @return True if execution is active.
             */
            bool is_running() const {
                return m_running;
            }
            
            /**
             * @brief Check whether the VM is in single-step debug mode.
             *
             * @return True if the execution policy is STEP.
             */
            bool is_debug() const {
                return m_expo == exec_policy::STEP;
            }

            /**
             * @brief Check whether memory has been allocated.
             *
             * @return True if memory is valid.
             */
            bool is_memory_valid() const {
                return m_memory != nullptr;
            }
           
            /**
             * @brief Check whether register storage has been allocated.
             *
             * @return True if registers are valid.
             */
            bool is_registers_valid() const {
                return m_registers != nullptr;
            }
           
            /**
             * @brief Check whether extension instructions are enabled.
             *
             * @return True if extension opcodes are enabled.
             */
            bool is_ext_enabled() const {
                return m_ext_enable;
            }

            /**
             * @brief Check whether execution is paused because of a breakpoint.
             *
             * @return True if paused on breakpoint.
             */
            bool is_paused_on_breakpoint() const {
                return m_paused_on_breakpoint;
            }

            /**
             * @brief Get the current execution policy.
             *
             * @return Active execution policy.
             */
            exec_policy get_exec_policy() const {
                return m_expo;
            }

            /**
             * @brief Get the current boot mode.
             *
             * @return Active boot mode.
             */
            boot_mode get_boot_mode() const {
                return m_boot_mode;
            }

            /**
             * @brief Get the active breakpoint set.
             *
             * @return Reference to the breakpoint bitset.
             */
            const breakpoints_t& breakpoints() {
                return m_breakpoints;
            }
            
            /**
             * @brief Read a word from VM memory.
             *
             * @param addr Address to read.
             * @param hk If true, dispatch any configured memory hooks.
             * @return Value stored at the address.
             */
            std_word_t mem_read(std_word_t addr, bool hk=true) {
                if (!m_memory) {
                    set_error(error_type::INVALID_MEMORY);
                    return 0;
                }

                if (m_hooks.before_memory_read && hk) {
                    m_hooks.before_memory_read(self, addr);
                }

                std_word_t value = m_memory[addr];
                
                if (m_hooks.after_memory_read && hk) {
                    m_hooks.after_memory_read(self, addr, value);
                }

                return value;
            }

            /**
             * @brief Read a register value.
             *
             * @param reg Register identifier.
             * @return Register contents.
             */
            std_word_t reg_read(registers reg) {
                if (!m_registers) {
                    set_error(error_type::INVALID_REGISTERS);
                    return 0;
                }

                std_word_t value = m_registers[(uint8_t)reg];

                return value;
            }

            /**
             * @brief Return the current program counter value.
             *
             * @return Current PC value.
             */
            std_word_t pc() {
                return reg_read(registers::PC);
            }

            /**
             * @brief Get the loaded memory sections.
             *
             * @return Read-only list of section descriptors.
             */
            const sections_addr& sections() {
                return m_sections;
            }

            /**
             * @brief Return the most recent VM error code.
             *
             * @return Last recorded error value.
             */
            error_type get_last_error() const {
                return m_last_error;   
            }

            /**
             * @brief Get the number of words currently occupied in memory.
             *
             * @return Used memory size in words.
             */
            std_word_t get_used_memory_size() const {
                return m_used_memory_size;
            }

            /**
             * @brief Check whether the VM is in a clean, no-error state.
             *
             * @return True when the last error is NO_ERROR.
             */
            bool ok() const {
                return m_last_error == error_type::NO_ERROR;
            }

            // ----------------- CTRL
            /**
             * @brief Access the VM hook container.
             *
             * @return Reference to the callback configuration object.
             */
            vm_hooks& hooks() {
                return m_hooks;
            }

            /**
             * @brief Clear the current error state.
             */
            void clear_error() {
                m_last_error = error_type::NO_ERROR;
            }

            /**
             * @brief Reset memory, registers, and execution state.
             */
            void reset() {
                // 1. break execution
                m_running = false;
                m_explicit_stop = false;

                // 2. reset memory (if reserved reset to 0 if not reserve it)
                reserve_memory();
                
                // 3. reset registers
                reserve_registers();

                // 4. state
                m_current_instruction = 0;
                m_paused_on_breakpoint = false;
                m_breakpoints.reset(); 
                m_sections.clear();
                clear_error();
                
                // 5. hook
                if (m_hooks.on_reset) m_hooks.on_reset(self);
            }

            /**
             * @brief Add a breakpoint at the given address.
             *
             * @param addr Memory address to break on.
             */
            void add_break_point(std_word_t addr) {
                m_breakpoints.set(addr);
            }

            /**
             * @brief Remove a breakpoint at the given address.
             *
             * @param addr Memory address to clear.
             */
            void remove_break_point(std_word_t addr) {
                m_breakpoints.reset(addr);
            }

            /**
             * @brief Load a program image from an input stream.
             *
             * @param stream Binary program data to load.
             */
            void load(std::istream& stream) {
                if (!stream) {
                    set_error(error_type::INVALID_MEMORY);
                    return;
                }

                if (!is_memory_valid()) {
                    if (!reserve_memory()) {
                        set_error(error_type::MEM_ALLOC_FAIL);
                        return;
                    }
                }
                
                if (!is_registers_valid()) {
                    if (!reserve_registers()) {
                        set_error(error_type::MEM_ALLOC_FAIL);
                        return;
                    }
                }

                // LC-3 object files are big-endian: first word is the .ORIG load
                // address, every word after that is loaded sequentially into memory
                // starting there.
                auto read_word = [&](std_word_t& out) -> bool {
                    char buf[2];

                    if (!stream.read(buf, 2)) {
                        return false;
                    }
                    
                    // swap to host byte order (big-endian -> little-endian)
                    out = (std_word_t)((uint8_t)buf[0] << 8 | (uint8_t)buf[1]);
                    return true;
                };

                if (!read_word(m_origin)) {
                    set_error(error_type::INVALID_MEMORY);
                    return;
                }

                std_word_t addr = m_origin;
                m_used_memory_size = 0;
                std_word_t word;

                while (read_word(word)) {
                    (self)[addr++] = word;
                    m_used_memory_size++;
                }

                m_sections.push_back({ m_origin, m_used_memory_size });
                set_pc(m_origin);
            }

            /**
             * @brief Start execution of the loaded program.
             *
             * @return Status code indicating execution result.
             */
            error_type run() {
                if (m_running) {
                    return error_type::ALREADY_RUNNING;
                }

                if (!is_memory_valid()) {
                    return error_type::INVALID_MEMORY;
                }

                m_running = true;

                // not paused due to a breakpoint and a hook is valid -> first time
                bool bped = handle_after_breakpoint();

                if (!bped && m_hooks.on_start) {
                    m_hooks.on_start(self);
                }

                // run just a setup for debug mode
                // in case of a breakpoint in debug mode, handling the BP inst equals a step so no need to continue.
                if (is_debug()) {
                    // if BP return execution result, if setup NO ERROR
                    return bped ? get_last_error() : error_type::NO_ERROR;
                }
                
                return run_();
            }

            /**
             * @brief Request an immediate stop to execution.
             */
            void stop() {
                stop_(true);
            }
            
            /**
             * @brief Execute one instruction in debug mode.
             *
             * @return Program counter value after the instruction executes.
             */
            std_word_t step() {
                if (!is_debug()) {
                    set_error(error_type::WRONG_EXEC_POLICY);
                    return 0;
                }

                if (!is_memory_valid()) {
                    set_error(error_type::INVALID_MEMORY);
                    return 0;
                }

                // no point stepping if already halted or errored from previous step
                if (!ok() || m_explicit_stop) {
                    return pc();
                }
                
                // check if current PC falls within any loaded section
                if (!is_in_bound()) {
                    stop_(true);
                    return pc();
                }

                service_interrupt();
                
                fetch_and_exec();

                if (!ok()) {
                    stop_(true);
                }

                return pc();
            }

            /**
             * @brief Destroy the VM and release any allocated memory/register storage.
             */
            ~VM() {
                // 1. stop execution cleanly
                if (m_running) {
                    m_running = false;
                    if (m_hooks.on_halt) {
                        m_hooks.on_halt(self, pc(), true);
                    }
                }

                // 2. free memory if allocated
                if (m_memory) {
                    delete[] m_memory;
                    m_memory = nullptr;
                }

                // 3. free registers if allocated
                if (m_registers) {
                    delete[] m_registers;
                    m_registers = nullptr;
                }
            }
    };
    
    #undef self
} // namespace lc3kit

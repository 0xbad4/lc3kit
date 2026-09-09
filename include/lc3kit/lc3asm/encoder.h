#pragma once

#include "symtab.h"

namespace lc3kit::lasm {

    /**
     * @brief One assembled section: its origin address and emitted words.
     */
    struct section { std_word_t  origin; std::vector<std_word_t> words; };

    /**
     * @brief Container of assembled sections produced by the pass-2 encoder.
     */
    using  sections_t = std::vector<section>;

    /**
     * @brief Encodes resolved instructions into machine words for each .ORIG section.
     */
    class Encoder : public Visitor, public BaseObj {
        protected:
            sections_t          m_sections  {};
            section*            m_current   = nullptr;
            const sym_table_t*  m_sym_table = nullptr;
            std_word_t          m_lc        = 0;
            tpos                m_current_pos = {0, 0};

            // emit
            void emit_word(std_word_t word) {
                if (!m_current) {
                    report(error_type::NO_ORIG, m_current_pos);
                    return;
                }
                m_current->words.push_back(word);
                m_lc++;
            }

            // operand resolution

            // resolve src2 to a raw uint16 value.
            // for labels: look up in symbol table.
            // for registers: return register index.
            // for immediate: return value directly.
            std_word_t resolve(const ins_src2_t& s2) {
                if (std::holds_alternative<registers>(s2))
                    return (std_word_t)std::get<registers>(s2);

                if (std::holds_alternative<std_word_t>(s2))
                    return std::get<std_word_t>(s2);

                if (std::holds_alternative<str_t>(s2)) {
                    const str_t& lbl = std::get<str_t>(s2);
                    auto it = m_sym_table->find(lbl);
                    if (it == m_sym_table->end()) {
                        report(error_type::UNDEFINED_LABEL, m_current_pos);
                        return 0;
                    }
                    return it->second;
                }

                // monostate - caller should not have called resolve on an empty operand
                report(error_type::UNEXPECTED_TOKEN, m_current_pos);
                return 0;
            }

            // compute PC-relative offset from an src2 (label or immediate).
            // offset = target - (current LC + 1)  - post-increment PC per spec.
            std_word_t pcoffset(const ins_src2_t& s2, uint8_t bits) {
                std_word_t target = resolve(s2);

                if (!ok()) {
                    return 0;
                }

                // both target and m_lc are 16-bit addresses -- subtraction stays 16-bit
                std_sword_t offset = (std_sword_t)(target - (m_lc + 1));

                std_sword_t lo = -(1 << (bits - 1));
                std_sword_t hi =  (1 << (bits - 1)) - 1;

                if (offset < lo || offset > hi) {
                    report(error_type::NUMBER_OUT_OF_RANGE, m_current_pos);
                    return 0;
                }

                std_word_t smv = mask((std_word_t)offset, bits);

                return smv;
            }

            // mask a value to n bits (strips sign for embedding in instruction word)
            static std_word_t mask(std_word_t val, uint8_t bits) {
                std_word_t m = bits >= 16 ? 0xFFFF : (std_word_t)((1u << bits) - 1);
                return val & m;
            }

            // sign-extend an immediate from src2, range-checked against bits
            std_word_t imm(const ins_src2_t& s2, uint8_t bits) {
                std_word_t raw = resolve(s2);

                if (!ok()) {
                    return 0;
                }

                std_sword_t sv  = (std_sword_t)raw;
                std_sword_t lo  = -(1 << (bits - 1));
                std_sword_t hi  =  (1 << (bits - 1)) - 1;

                if (sv < lo || sv > hi) {
                    report(error_type::NUMBER_OUT_OF_RANGE, m_current_pos);
                    return 0;
                }

                std_word_t smv = mask((std_word_t)sv, bits);

                return smv;
            }

            // src2 can be a register index or an immediate - used by ADD/AND
            std_word_t src2_reg_or_imm(const ins_src2_t& s2, uint8_t imm_bits) {
                if (std::holds_alternative<registers>(s2)) {
                    // register mode: bit[5]=0, bits[2:0]=SR2
                    return (std_word_t)std::get<registers>(s2) & 0x7;
                }
                // immediate mode: bit[5]=1, bits[imm_bits-1:0]=imm
                return (1 << imm_bits) | imm(s2, imm_bits);
            }

            void emit_ext(uint8_t op, InsArithmetic* ins) {
                std_word_t lower;

                if (std::holds_alternative<registers>(ins->src2)) {
                    // register mode: bit[3]=1, bits[2:0]=SR2
                    lower = (1 << 3) | ((std_word_t)std::get<registers>(ins->src2) & 0x7);
                } 
                else {
                    // immediate mode: bit[3]=0, bits[2:0]=imm3
                    lower = imm(ins->src2, 3);
                    
                    if (!ok()) {
                        return;
                    }
                }

                std_word_t word = (0b1101                    << 12)
                                | ((std_word_t)op            << 10)
                                | ((std_word_t)ins->dest_reg << 7)
                                | ((std_word_t)ins->src_reg1 << 4)
                                | lower;
                emit_word(word);
            }

        public:
            using BaseObj::BaseObj;

            void encode(const instructions& instructions, const sym_table_t& sym_table) {
                m_sym_table = &sym_table;
                m_sections.clear();
                m_current = nullptr;
                m_lc = 0;

                start();

                for (auto& ins : instructions) {
                    m_current_pos = ins->pos;

                    ins->accept(this);

                    if (!ok()) {
                        return;
                    }
                }

                stop();
            }
            
            const sections_t& sections()  const {
                return m_sections;
            }

            // + arithmetic

            // ADD DR, SR1, SR2/imm5
            // 0001 | DR[3] | SR1[3] | 0 00 SR2[3]   (register mode)
            // 0001 | DR[3] | SR1[3] | 1 imm5[5]     (immediate mode)
            void visit(InsADD* ins) {
                std_word_t word = (0b0001 << 12)
                                | ((std_word_t)ins->dest_reg  << 9)
                                | ((std_word_t)ins->src_reg1  << 6)
                                | src2_reg_or_imm(ins->src2, 5);
                emit_word(word);
            }

            // AND DR, SR1, SR2/imm5  - identical layout to ADD, different opcode
            void visit(InsAND* ins) {
                std_word_t word = (0b0101 << 12)
                                | ((std_word_t)ins->dest_reg  << 9)
                                | ((std_word_t)ins->src_reg1  << 6)
                                | src2_reg_or_imm(ins->src2, 5);
                emit_word(word);
            }

            // NOT DR, SR
            // 1001 | DR[3] | SR[3] | 111111
            void visit(InsNOT* ins) {
                std_word_t word = (0b1001 << 12)
                                | ((std_word_t)ins->dest_reg << 9)
                                | ((std_word_t)ins->src_reg  << 6)
                                | 0b111111;
                emit_word(word);
            }

            // + lc3ext-ext
            void visit(InsSHL* ins) { 
                emit_ext(0b00, ins);
            }

            void visit(InsSHR* ins) { 
                emit_ext(0b01, ins);
            }

            void visit(InsMUL* ins) { 
                emit_ext(0b10, ins);
            }

            void visit(InsDIV* ins) { 
                emit_ext(0b11, ins);
            }

            // + branch

            // BR NZP, pcoffset9
            // 0000 | NZP[3] | pcoffset9[9]
            void visit(InsBR* ins) {
                std_word_t off = pcoffset(ins->offset, 9);
                IS_OK

                std_word_t word = (0b0000 << 12)
                                | ((std_word_t)ins->nzp << 9)
                                | mask(off, 9);
                emit_word(word);
            }

            // + load

            // LD DR, pcoffset9
            // 0010 | DR[3] | pcoffset9[9]
            void visit(InsLD* ins) {
                std_word_t off = pcoffset(ins->offset, 9);
                IS_OK
                emit_word((0b0010 << 12) | ((std_word_t)ins->dest_reg << 9) | mask(off, 9));
            }

            // LDI DR, pcoffset9
            // 1010 | DR[3] | pcoffset9[9]
            void visit(InsLDI* ins) {
                std_word_t off = pcoffset(ins->offset, 9);
                IS_OK
                emit_word((0b1010 << 12) | ((std_word_t)ins->dest_reg << 9) | mask(off, 9));
            }

            // LEA DR, pcoffset9
            // 1110 | DR[3] | pcoffset9[9]
            void visit(InsLEA* ins) {
                std_word_t off = pcoffset(ins->offset, 9);
                IS_OK
                emit_word((0b1110 << 12) | ((std_word_t)ins->dest_reg << 9) | mask(off, 9));
            }

            // LDR DR, BaseR, offset6
            // 0110 | DR[3] | BaseR[3] | offset6[6]
            void visit(InsLDR* ins) {
                std_word_t off = imm(ins->offset, 6);
                IS_OK
                emit_word(
                    (0b0110 << 12)
                    | ((std_word_t)ins->reg      << 9)
                    | ((std_word_t)ins->base_reg << 6)
                    | off
                );
            }

            // + store

            // ST SR, pcoffset9
            // 0011 | SR[3] | pcoffset9[9]
            void visit(InsST* ins) {
                std_word_t off = pcoffset(ins->offset, 9);
                IS_OK
                emit_word((0b0011 << 12) | ((std_word_t)ins->dest_reg << 9) | mask(off, 9));
            }

            // STI SR, pcoffset9
            // 1011 | SR[3] | pcoffset9[9]
            void visit(InsSTI* ins) {
                std_word_t off = pcoffset(ins->offset, 9);
                IS_OK
                emit_word((0b1011 << 12) | ((std_word_t)ins->dest_reg << 9) | mask(off, 9));
            }

            // STR SR, BaseR, offset6
            // 0111 | SR[3] | BaseR[3] | offset6[6]
            void visit(InsSTR* ins) {
                std_word_t off = imm(ins->offset, 6);
                IS_OK
                emit_word((0b0111 << 12)
                | ((std_word_t)ins->reg      << 9)
                | ((std_word_t)ins->base_reg << 6)
                | off);
            }

            // + control flow

            // JMP BaseR
            // 1100 | 000 | BaseR[3] | 000000
            void visit(InsJMP* ins) {
                emit_word((0b1100 << 12) | ((std_word_t)ins->base_reg << 6));
            }

            // JSR pcoffset11
            // 0100 | 1 | pcoffset11[11]
            void visit(InsJSR* ins) {
                std_word_t off = pcoffset(ins->offset, 11);
                IS_OK
                emit_word((0b0100 << 12) | (1 << 11) | mask(off, 11));
            }

            // JSRR BaseR
            // 0100 | 0 | 00 | BaseR[3] | 000000
            void visit(InsJSRR* ins) {
                emit_word((0b0100 << 12) | ((std_word_t)ins->base_reg << 6));
            }

            // RET - syntactic sugar for JMP R7
            // 1100 | 000 | 111 | 000000
            void visit(InsRET*) {
                emit_word((0b1100 << 12) | (7 << 6));
            }

            // RTI
            // 1000 | 000000000000
            void visit(InsRTI*) {
                emit_word(0b1000 << 12);
            }

            void visit(InsNOP* ins) {
                (void)ins;
                // label carrier only - emits no words 
            }

            // + trap

            // TRAP trapvect8
            // 1111 | 0000 | trapvect8[8]
            void visit(InsTRAP* ins) {
                emit_word((0b1111 << 12) | (ins->vector & 0xFF));
            }

            // + directives

            // .ORIG - open a new section, reset LC, emit no words
            void visit(InsORIG* ins) {
                std_word_t origin = std::get<std_word_t>(ins->value);
                m_sections.push_back({ origin, {} });
                m_current = &m_sections.back();
                m_lc      = origin;
            }

            // .END - close current section, emit no words
            void visit(InsEND*) {
                m_current = nullptr;
            }

            // .FILL value - one word, value or resolved label
            void visit(InsFILL* ins) {
                emit_word(resolve(ins->value));
            }

            // .BLKW n - n zero words
            void visit(InsBLKW* ins) {
                std_word_t count = std::get<std_word_t>(ins->value);
                for (std_word_t i = 0; i < count; i++)
                    emit_word(0x0000);
            }

            // .STRINGZ "text" - one word per char + null terminator
            void visit(InsSTRINGZ* ins) {
                for (char c : ins->text)
                    emit_word((std_word_t)c);
                emit_word(0x0000);  // null terminator
            }
        };

} // namespace lc3kit::lasm
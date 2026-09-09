#pragma once

#include "parser.h"

namespace lc3kit::lasm {
    /**
     * @brief Maps labels to their assigned memory addresses.
     */
    using sym_table_t = std::unordered_map<str_t, std_word_t>;
    
    /**
     * @brief Pass-1 assembler stage that assigns addresses and resolves labels.
     */
    class SymTableGenerator : public Visitor, public BaseObj {
        protected:
            sym_table_t m_sym_table {};
            std_word_t  m_lc        = 0;  // location counter
            tpos        m_current_pos = {0, 0};

            // record label at current LC if the instruction carries one
            void record(Instruction* ins) {
                ins->address = m_lc;   // stamp address onto the node
                if (!ins->label.empty())
                    m_sym_table[ins->label] = m_lc;
            }

            // advance LC by n words after processing an instruction
            void advance(std_word_t n = 1) { 
                m_lc += n; 
            }

        public:
            using BaseObj::BaseObj;

            // NOTE: no need to verify if lc3kit-ext is enabled.
            SymTableGenerator() = default;

            // query
            const sym_table_t& get() const { return m_sym_table; }

            // returns 0 and sets error if label not found
            std_word_t find(const str_t& lbl) {
                auto it = m_sym_table.find(lbl);

                if (it == m_sym_table.end()) {
                    report(error_type::UNDEFINED_LABEL, m_current_pos);
                    return 0;
                }
                return it->second;
            }

            std_word_t operator[] (const str_t& lbl) {
                return find(lbl);
            }

            void generate(const instructions& instructions) {
                reset();
                m_sym_table.clear();
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

            // each is 1 word
            void visit(InsADD* ins) { 
                record(ins); 
                advance(); 
            }
            
            void visit(InsAND* ins) { 
                record(ins); 
                advance(); 
            }
            
            void visit(InsNOT* ins) { 
                record(ins); 
                advance(); 
            }

            void visit(InsSHL* ins) {
                record(ins);
                advance();
            }
            
            void visit(InsSHR* ins) {
                record(ins);
                advance();
            }
            
            void visit(InsMUL* ins) {
                record(ins);
                advance();
            }
            
            void visit(InsDIV* ins) {
                record(ins);
                advance();
            }

            void visit(InsBR* ins) { 
                record(ins); 
                advance(); 
            }
            
            void visit(InsLD*  ins) { 
                record(ins); 
                advance(); 
            }
            
            void visit(InsLDI* ins) { 
                record(ins); 
                advance(); 
            }
            
            void visit(InsLDR* ins) { 
                record(ins); 
                advance(); 
            }
            
            void visit(InsLEA* ins) { 
                record(ins); 
                advance(); 
            }
            
            void visit(InsST*  ins) { 
                record(ins); 
                advance(); 
            }
            
            void visit(InsSTI* ins) { 
                record(ins); 
                advance(); 
            }
            
            void visit(InsSTR* ins) { 
                record(ins); 
                advance(); 
            }
            
            void visit(InsJMP*  ins) { 
                record(ins); 
                advance(); 
            }
            
            void visit(InsJSR*  ins) { 
                record(ins); 
                advance(); 
            }
            
            void visit(InsJSRR* ins) { 
                record(ins); 
                advance(); 
            }
            
            void visit(InsRET*  ins) { 
                record(ins); 
                advance(); 
            }
            
            void visit(InsRTI*  ins) { 
                record(ins); 
                advance(); 
            }
            
            void visit(InsTRAP* ins) { 
                record(ins); 
                advance(); 
            }

            void visit(InsNOP* ins) override { 
                record(ins); 
            }

            // .ORIG resets the location counter - no word emitted
            void visit(InsORIG* ins) override {
                if (!std::holds_alternative<std_word_t>(ins->value)) {
                    report(error_type::EXPECTED_NUMBER, m_current_pos);
                    return;
                }
                m_lc = std::get<std_word_t>(ins->value);
                record(ins);  // label on .ORIG gets the origin address
            }

            // .END - no word emitted, stop is handled by run() reaching the end
            void visit(InsEND* ins) override { 
                record(ins); 
            }

            // .FILL - 1 word
            void visit(InsFILL* ins) override { 
                record(ins); 
                advance(); 
            }

            // .BLKW n - n words of zeroed memory
            void visit(InsBLKW* ins) override {
                record(ins);
                if (!std::holds_alternative<std_word_t>(ins->value)) {
                    report(error_type::EXPECTED_NUMBER, m_current_pos);
                    return;
                }
                advance(std::get<std_word_t>(ins->value));
            }

            // .STRINGZ "text" - len(text) + 1 words (null terminator)
            void visit(InsSTRINGZ* ins) override {
                record(ins);
                advance((std_word_t)(ins->text.size() + 1));
            }
    };
} // namespace lc3kit::lasm

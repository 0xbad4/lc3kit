#pragma once

#include "expr.h"


namespace lc3kit::lasm {
    /**
     * @brief Parses a token stream into instruction nodes and directive objects.
     */
    class Parser : public BaseObj{
        protected:
            instructions    m_inst    {};
            const tokens_t* m_tokens  { nullptr };
            uint               m_index = 0;

            // -------------- utilities
            bool is_end() { 
                return (m_index >= m_tokens->size()) || (peek().type == token_type::EOF_); 
            }

            bool is_trap_alias(const str_t& mnemonic) {
                return TRAP_VECTORS.count(mnemonic) > 0;
            }

            bool is_branch(const str_t& mnemonic) {
                return BR_MASKS.count(mnemonic) > 0;
            }

            const token& peek() {
                if (m_index >= m_tokens->size()) {
                    return m_tokens->back();
                }
                return m_tokens->at(m_index);
            }

            // NOTE: errors in parser are fatal
            void report(error_type et) {
                BaseObj::report(et, peek().pos);
                m_is_running = false;
            }

            void consume() {
                m_index++;
            }

            // current token's type matches at least one type 
            bool match(std::unordered_set<token_type> types) {
                for (auto& type : types) {
                    if (match(type)) {
                        return true;
                    }
                }

                return false;
            }

            bool match(token_type tt) {
                return peek().type == tt;
            }

            // consume current, expect `type` and consume next (if consume_ is set)
            void expect(token_type tt, error_type et = error_type::UNEXPECTED_TOKEN, bool consume_=true) {
                consume();

                if (!match(tt)) {
                    report(et);
                }

                if (consume_) {
                    consume();
                }
            }

            // NOTE: get_* methods do not consume
            
            // is_signed: whether to allow negative numbers (default false)
            // sign_err: whether to report errors on sign mismatch (default false)
            std_word_t get_number(uint8_t bits = 16, bool is_signed = false, bool sign_err = false) {
                std_word_t value = 0;
                bool negative = false;

                // NOTE: hex values can not carry a sign prefix.
                if (match(token_type::HEX_NUMBER)) {
                    // Format: "x..." or "X..."
                    str_t hex_str = peek().value;
                    value = std::stoul(hex_str.substr(1), nullptr, 16);
                } 
                
                else if (match(token_type::DEC_NUMBER)) {
                    const str_t& raw = peek().value;
                    negative = raw[0] == '-';
                    value    = (std_word_t)std::stoul(negative ? raw.substr(1) : raw, nullptr, 10);
                } 
                
                else {
                    report(error_type::EXPECTED_NUMBER);
                    return 0;
                }

                bool treat_as_signed = negative || is_signed;
                bool sign_mismatch = negative && !is_signed;

                if (sign_mismatch && sign_err) {
                    report(error_type::INVALID_SIGN);
                    return 0;
                }

                if (treat_as_signed) {
                    int32_t lo  = -(1 << (bits - 1));
                    int32_t hi  =  (1 << (bits - 1)) - 1;
                    int32_t sv  =  negative ? -(int32_t)value : (int32_t)value;

                    if (sv < lo || sv > hi) {
                        report(error_type::NUMBER_OUT_OF_RANGE);
                        return 0;
                    }
                    // two's complement encode into the requested bit width
                    return (std_word_t)sv;
                } 
                else {
                    std_word_t hi = (bits >= 16)
                                    ? std::numeric_limits<std_word_t>::max()
                                    : (std_word_t)((1u << bits) - 1);
                    if (value > hi) {
                        report(error_type::NUMBER_OUT_OF_RANGE);
                        return 0;
                    }
                    return value;
                }
            }

            // err=true reports error and stops parsing
            registers get_register(bool err=true) {
                if (peek().type != token_type::IDENTIFIER) {
                    if (err) {
                        report(error_type::EXPECTED_REGISTER);
                    }
                    return registers::NOR;
                }

                str_t reg = peek().value;
                
                if (reg.size() != 2) {
                    if (err) {
                        report(error_type::INVALID_REGISTER_FORMAT);
                    }
                    return registers::NOR;
                }

                if (std::toupper(reg.at(0)) != 'R') {
                    if (err) {
                        report(error_type::INVALID_REGISTER_FORMAT);
                    }
                    return registers::NOR;
                }

                
                uint8_t num = reg.at(1) - '0';
                
                if (num <= 7) {
                    return (registers)num;
                }
                else {
                    if (err) {
                        report(error_type::REGISTER_OUT_OF_RANGE);
                    }
                    return registers::NOR;
                }
            }

            // parse a pcoffset or label - anything that can appear after a reg operand
            // in LD/ST/BR/JSR etc.  must be a number or a label.
            ins_src2_t get_offset() {
                if (match({ token_type::HEX_NUMBER, token_type::DEC_NUMBER }))
                    return get_number();

                if (match(token_type::IDENTIFIER))
                    return peek().value;

                report(error_type::EXPECTED_LABEL_OR_NUMBER);
                return {};
            }

            void insert(instruction_ptr ins) {
                m_inst.push_back(MV(ins));
            }

            // -------------- parsers
            void parse_line() {
                str_t label;

                // if the first token on a line is an identifier that isn't a known
                // mnemonic or directive, treat it as a label.
                if (match(token_type::IDENTIFIER) && !is_branch(peek().value) && !is_trap_alias(peek().value)) {
                    label = peek().value;
                    tpos p = peek().pos;

                    consume();  // label

                    if (match(token_type::EOL) || is_end()) {
                        // label-only line -- emit NOP as label carrier, no words emitted
                        insert(ins_uptr(InsNOP)(label, p));

                        if (match(token_type::EOL)) { 
                            consume(); 
                        }

                        // parse next line -- it may also be label-only or a real statement
                        if (!is_end()) parse_line();
                        return;
                    }
                }

                if (!is_end() && !match(token_type::EOL))
                    parse_statement(MV(label));
            }
            
            void parse_statement(str_t label) {
                token_type tt = peek().type;

                if (match({token_type::ADD, token_type::AND}))
                    return parse_arithmetic(MV(label));

                if (match({
                        token_type::SHL, 
                        token_type::SHR, 
                        token_type::MUL, 
                        token_type::DIV
                    }) && is_ext_enabled()) {
                        return parse_arithmetic(MV(label), true);
                    }

                if (tt == token_type::NOT)
                    return parse_not(MV(label));

                if (is_branch(peek().value))
                    return parse_branch(MV(label));

                if (match({
                        token_type::LD,  token_type::LEA, token_type::STI,
                        token_type::LDI, token_type::ST 
                    }))
                    return parse_pcoffset9(MV(label));
                
                if (tt == token_type::LDR || tt == token_type::STR)
                    return parse_base_offset(MV(label));

                if (tt == token_type::JMP)
                    return parse_jmp(MV(label));

                if (tt == token_type::JSR)
                    return parse_jsr(MV(label));

                if (tt == token_type::JSRR)
                    return parse_jsrr(MV(label));

                if (tt == token_type::RET)
                    return parse_ret(MV(label));

                if (tt == token_type::RTI)
                    return parse_rti(MV(label));

                if (tt == token_type::TRAP || is_trap_alias(peek().value))
                    return parse_trap(MV(label));

                if (tt == token_type::ORIG)   
                    return parse_orig(MV(label));

                if (tt == token_type::END)    
                    return parse_end(MV(label));

                if (tt == token_type::FILL)   
                    return parse_fill(MV(label));

                if (tt == token_type::BLKW)   
                    return parse_blkw(MV(label));

                if (tt == token_type::STRINGZ)
                    return parse_stringz(MV(label));
                    
                report(error_type::UNEXPECTED_TOKEN);

            }

            // ADD, AND
            // i.g ADD R0, R1, LABEL
            // + EXT: MUL, DIV, SHL, SHR
            void parse_arithmetic(str_t label, bool ext=false) {
                token_type opt = peek().type;
                consume();  // mnemonic

                // next is destination register
                registers dest_reg = get_register();
                IS_OK

                expect(token_type::COMMA, error_type::EXPECTED_COMMA); // consume destination register and expect comma
                IS_OK

                registers src_reg1 = get_register();
                IS_OK

                expect(token_type::COMMA, error_type::EXPECTED_COMMA); // consume source register 1 and expect comma
                IS_OK

                ins_src2_t last_operand {};

                if (match({token_type::HEX_NUMBER, token_type::DEC_NUMBER})) {
                    // NOTE: lc3kit-ext instructions supports only immediate value 3bit long
                    last_operand = get_number((ext ? 3 : 5), true);
                }
                // register or label
                else if (match(token_type::IDENTIFIER)) {
                    // assume its a register by default (do not raise an error)
                    registers  src_reg2 = get_register(false);

                    if (src_reg2 != registers::NOR) {
                        last_operand = src_reg2;
                    }
                    // not a register, its a label
                    else {
                        last_operand = peek().value;
                    }
                }
                else {
                    report(error_type::UNEXPECTED_TOKEN);
                }

                IS_OK

                consume(); // last operand

                tpos p = peek().pos;

                switch (opt) {
                    case token_type::AND:
                        insert(ins_uptr(InsAND)(dest_reg, src_reg1, last_operand, label, p));
                        break;
                    case token_type::ADD:
                        insert(ins_uptr(InsADD)(dest_reg, src_reg1, last_operand, label, p));
                        break;
                    case token_type::SHL:
                        insert(ins_uptr(InsSHL)(dest_reg, src_reg1, last_operand, label, p));
                        break;
                    case token_type::SHR:
                        insert(ins_uptr(InsSHR)(dest_reg, src_reg1, last_operand, label, p));
                        break;
                    case token_type::MUL:
                        insert(ins_uptr(InsMUL)(dest_reg, src_reg1, last_operand, label, p));
                        break;
                    case token_type::DIV:
                        insert(ins_uptr(InsDIV)(dest_reg, src_reg1, last_operand, label, p));
                        break;
                }

            }

            // DR, SR  (two registers, no third operand)
            void parse_not(str_t label) {
                tpos p = peek().pos;

                consume();  // mnemonic

                registers dr = get_register();
                IS_OK

                expect(token_type::COMMA, error_type::EXPECTED_COMMA);
                IS_OK

                registers sr = get_register();
                IS_OK

                consume();  // SR

                insert(ins_uptr(InsNOT)(dr, sr, label, p));
            }

            void parse_branch(str_t label) {
                tpos p = peek().pos;
                
                uint8_t mask = BR_MASKS.at(peek().value);

                consume();  // mnemonic

                ins_src2_t offset = get_offset();
                IS_OK

                consume();  // offset / label

                insert(ins_uptr(InsBR)(mask, offset, label, p));
            }

            // LD, LDI, LEA, ST, STI
            // reg, pcoffset9 / label
            void parse_pcoffset9(str_t label) {
                token_type opt = peek().type;
                tpos p = peek().pos;
                
                consume();  // mnemonic

                registers reg = get_register();
                IS_OK

                expect(token_type::COMMA, error_type::EXPECTED_COMMA);
                IS_OK

                ins_src2_t offset = get_offset();
                IS_OK

                consume();  // offset

                switch (opt) {
                    case token_type::LD:
                        insert(ins_uptr(InsLD)(reg, offset, label, p));  break;
                    case token_type::LDI:
                        insert(ins_uptr(InsLDI)(reg, offset, label, p)); break;
                    case token_type::LEA:
                        insert(ins_uptr(InsLEA)(reg, offset, label, p)); break;
                    case token_type::ST:
                        insert(ins_uptr(InsST)(reg, offset, label, p));  break;
                    case token_type::STI:
                        insert(ins_uptr(InsSTI)(reg, offset, label, p)); break;
                    default: break;
                }
            }

            // reg, BaseR, offset6
            void parse_base_offset(str_t label) {
                token_type opt = peek().type;
                tpos p = peek().pos;
                
                consume();  // mnemonic

                registers reg = get_register();
                IS_OK

                expect(token_type::COMMA, error_type::EXPECTED_COMMA);
                IS_OK

                registers base = get_register();
                IS_OK

                expect(token_type::COMMA, error_type::EXPECTED_COMMA);
                IS_OK

                ins_src2_t offset = get_offset();
                IS_OK

                consume();  // offset

                if (opt == token_type::LDR)
                    insert(ins_uptr(InsLDR)(reg, base, offset, label, p));
                else
                    insert(ins_uptr(InsSTR)(reg, base, offset, label, p));
            }

            // JMP
            // BaseR
            void parse_jmp(str_t label) {
                tpos p = peek().pos;
                consume();  // mnemonic

                registers base = get_register();
                IS_OK

                consume();  // BaseR

                insert(ins_uptr(InsJMP)(base, label, p));
            }

            // JSR
            // pcoffset11 / label
            void parse_jsr(str_t label) {
                tpos p = peek().pos;
                
                consume();  // mnemonic

                ins_src2_t offset = get_offset();
                IS_OK

                consume();  // offset

                insert(ins_uptr(InsJSR)(offset, label, p));
            }

            // JSRR
            // BaseR
            void parse_jsrr(str_t label) {
                tpos p = peek().pos;
                
                consume();  // mnemonic

                registers base = get_register();
                IS_OK

                consume();  // BaseR

                insert(ins_uptr(InsJSRR)(base, label, p));
            }

            // RET
            // no operands - syntactic sugar for JMP R7
            void parse_ret(str_t label) {
                tpos p = peek().pos;
                
                consume();  // mnemonic
                insert(ins_uptr(InsRET)(label, p));
            }

            // RTI
            // no operands
            void parse_rti(str_t label) {
                tpos p = peek().pos;
                
                consume();  // mnemonic
                insert(ins_uptr(InsRTI)(label, p));
            }

            // TRAP trapvector8, or alias (GETC/OUT/PUTS/IN/PUTSP/HALT)
            // aliases are resolved to their vector here so the encoder sees InsTRAP
            void parse_trap(str_t label) {
                tpos p = peek().pos;
                
                std_word_t vector = 0;

                if (is_trap_alias(peek().value)) {
                    // alias: vector known from the mnemonic itself
                    vector = TRAP_VECTORS.at(peek().value);
                    consume();  // alias mnemonic
                } else {
                    // explicit TRAP xNN
                    consume();  // TRAP mnemonic

                    if (!match({ token_type::HEX_NUMBER, token_type::DEC_NUMBER })) {
                        report(error_type::EXPECTED_NUMBER);
                        return;
                    }
                    vector = get_number(8, false);
                    consume();  // vector
                }

                insert(ins_uptr(InsTRAP)(vector, label, p));
            }

            // directives

            // .ORIG address
            void parse_orig(str_t label) {
                tpos p = peek().pos;
                
                consume();  // .ORIG

                if (!match({ token_type::HEX_NUMBER, token_type::DEC_NUMBER })) {
                    report(error_type::EXPECTED_NUMBER);
                    return;
                }
                ins_src2_t addr = get_number(16, false, true);

                consume();  // address

                insert(ins_uptr(InsORIG)(addr, label, p));                
            }

            // .END - no operand
            void parse_end(str_t label) {
                tpos p = peek().pos;
                
                consume();  // .END
                insert(ins_uptr(InsEND)(label, p));
            }

            // .FILL value (number or label)
            void parse_fill(str_t label) {
                tpos p = peek().pos;
                
                consume();  // .FILL

                ins_src2_t val = get_offset();  // number or label
                if (!ok()) return;
                consume();  // value

                insert(ins_uptr(InsFILL)(val, label, p));
            }

            // .BLKW count (number only)
            void parse_blkw(str_t label) {
                tpos p = peek().pos;
                
                consume();  // .BLKW

                if (!match({ token_type::HEX_NUMBER, token_type::DEC_NUMBER })) {
                    report(error_type::EXPECTED_NUMBER);
                    return;
                }
                ins_src2_t count = get_number(16, false, true);
                consume();  // count

                insert(ins_uptr(InsBLKW)(count, label, p));
            }

            // .STRINGZ "text"
            void parse_stringz(str_t label) {
                tpos p = peek().pos;
                
                consume();  // .STRINGZ

                if (!match(token_type::STRING)) {
                    report(error_type::EXPECTED_STRING);
                    return;
                }
                str_t text = peek().value;
                consume();  // string

                insert(ins_uptr(InsSTRINGZ)(text, label, p));
            }

        public:
            using BaseObj::BaseObj;
            
            void parse(const tokens_t& tokens) {
                reset();
                m_tokens = &tokens;
                m_index  = 0;
                m_inst.clear();

                start();

                while (!is_end() && is_running()) {
                    parse_line();

                    if (!ok()) {
                        break;
                    }

                    // each statement must end with EOL or EOF
                    if (!match(token_type::EOL) && !is_end()) {
                        report(error_type::EXPECTED_EOL);
                        break;
                    }

                    consume();  // EOL
                }
                m_tokens = nullptr;
                stop();
            }

            const instructions& get_instructions() const {
                return m_inst;
            }
    };
}

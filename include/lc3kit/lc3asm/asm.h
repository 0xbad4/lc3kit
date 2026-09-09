#pragma once

#include "lexer.h"
#include "parser.h"
#include "symtab.h"
#include "encoder.h"

#include <iostream>

namespace lc3kit::lasm {
    /**
     * @brief Maps source line numbers to assembled memory addresses.
     */
    using lines_addrs_t = std::unordered_map<uint, std_word_t>;

    /**
     * @brief High-level assembler pipeline for tokenization, parsing, symbol resolution,
     * and instruction encoding.
     */
    class Asm {
        protected:
            Lexer             m_lexer   {};
            Parser            m_parser  {};
            SymTableGenerator m_symgen  {};
            Encoder           m_encoder {};
            errors_t          m_errors  {};
            bool              m_ext   = false;

            // collect errors from a stage into m_errors
            // returns false if the stage had errors
            bool collect(const errors_t& errs) {
                if (errs.empty()) {
                    return true;
                }

                m_errors.insert(m_errors.end(), errs.begin(), errs.end());
                
                return false;
            }

        public:
            void set_ext_enabled(bool enabled) { 
                m_ext = enabled;
                m_lexer.set_ext_enabled(enabled);
                m_parser.set_ext_enabled(enabled);
                m_encoder.set_ext_enabled(enabled);
            }
            
            bool is_ext_enabled()        const { 
                return m_ext;    
            }

            bool                ok()        const { return m_errors.empty(); }
            const errors_t&     errors()    const { return m_errors; }
            const sym_table_t&  sym_table() const { return m_symgen.get(); }
            const sections_t&   sections()  const { return m_encoder.sections(); }

            lines_addrs_t line_addresses() const {
                lines_addrs_t result;
                
                for (const auto& ins : m_parser.get_instructions()) {
                    if (ins->pos.line > 0 && ins->address > 0) {
                        result[(uint)ins->pos.line] = ins->address;
                    }
                }
                return result;
            }
            
            // -- pipeline

            // assemble from a source_code struct
            bool assemble(source_code src) {
                m_errors  = {};
                
                // stage 1 - lex
                m_lexer.tokenize(src);

                if (!collect(m_lexer.errors())) {
                    return false;
                }

                // stage 2 - parse
                m_parser.parse(m_lexer.tokens());

                if (!collect(m_parser.errors())) {
                    return false;
                }

                
                // stage 3 - pass 1: build symbol table
                m_symgen.generate(m_parser.get_instructions());
                
                if (!collect(m_symgen.errors())) {
                    return false;
                }

                // stage 4 - pass 2: encode
                m_encoder.encode(m_parser.get_instructions(), m_symgen.get());

                if (!collect(m_encoder.errors())) {
                    return false;
                }

                return true;
            }

            // assemble from an already-opened stream
            bool assemble(std::istream& stream) {
                std::string content((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());

                return assemble({ content.c_str(), (uint32_t)content.size() });
            }
                
            // -- output

            // dump assembled sections as big-endian binary to an ostream
            void dump(std::ostream& out) const {
                for (const auto& section : m_encoder.sections()) {
                    uint16_t origin = section.origin;
                    out.put((char)(origin >> 8));
                    out.put((char)(origin & 0xFF));

                    for (std_word_t word : section.words) {
                        out.put((char)(word >> 8));
                        out.put((char)(word & 0xFF));
                    }
                }
            }

        };

} // namespace lc3kit::lasm
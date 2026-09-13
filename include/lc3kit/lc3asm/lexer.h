#pragma once

#include "bob.h"
#include <algorithm>

namespace lc3kit::lasm {
    /**
     * @brief A single token emitted by the lexer.
     */
    struct token {
        const token_type type;
        const str_t      value;
        const tpos       pos;

        token(token_type tt, tpos tp, str_t tv="") : type(tt), value(tv), pos(tp) {}
    };

    /**
     * @brief Vector of lexer output tokens.
     */
    using tokens_t = std::vector<token>;

    /**
     * @brief Tokenizes assembly source into a stream of typed tokens and diagnostics.
     */
    class Lexer : public BaseObj {
        protected:
            source_code m_src;
            tpos         m_current_pos;  // line:col
            uint           m_current;      // consider file as flat
            uint           m_start;
            tokens_t    m_tokens {};

            // ---------- utilities
            bool is_end() {
                return (m_current >= m_src.size);
            }

            bool is_digit(char ch) {
                return std::isdigit(ch);
            }
            
            bool is_alpha(char ch) {
                return std::isalpha(ch) || ch == '_';
            }
            
            bool is_almun(char ch) {
                return this->is_alpha(ch) || this->is_digit(ch);
            }

            bool is_hex(const str_t& str) {
                if (str.size() < 2)
                    return false;

                if (str[0] != 'x' && str[0] != 'X')
                    return false;

                for (std::size_t i = 1; i < str.size(); ++i) {
                    if (!std::isxdigit(static_cast<unsigned char>(str[i])))
                        return false;
                }

                return true;
            }

            // increment
            char advance() {
                m_current_pos.col += 1;
                return m_src.data[m_current++];
            }

            // does not increment
            char peek() {
                if (is_end()) {
                    return '\0';
                }
                return m_src.data[m_current];
            }

            void add_token(token_type tt, str_t lexeme = "") {
                m_tokens.push_back(token(tt, m_current_pos, lexeme));
            }

            void report(error_type et) {
                BaseObj::report(et, m_current_pos);
            }

            str_t get_current_value() {
                // get the stirng's value
                size_t ssize = m_current - m_start;

                str_t ss(this->m_src.data + m_start, ssize);

                return ss;
            }

            void new_line() {
                m_current_pos.col  = 0;
                m_current_pos.line += 1;
            }

            // ---------- handlers
            void handle_token() {
                char cc = advance();

                switch (cc) {
                    case ',':
                        add_token(token_type::COMMA, ",");
                        break;
                    case '.':
                        handle_directive();
                        break;
                    case '-':
                        handle_dec_number(true);
                        break;
                    case '#':
                        handle_dec_number();
                        break;
                    case '"':
                        handle_string();
                        break;
                    case ';':
                        handle_comment();
                        break;
                    case ' ':
                    case '\r':
                    case '\t':
                        // ignore whitespace.
                        break;
                    case END_OF_LINE:
                        handle_new_line();
                        break;
                    
                    default:
                        if (is_alpha(cc)) {
                            handle_identifier();
                        }
                        else if (is_digit(cc)) {
                            handle_bare_decimal();
                        }
                        else {
                            report(error_type::UNEXPECTED_CHAR);
                        }
                        break;
                }
            }

            void handle_comment() {
                // keep consuming until EOF hit
                while (peek() != '\n' && !is_end()) {
                    advance();
                }
            }

            void handle_new_line() {
                // collapse consecutive newlines -- only insert if last token wasn't EOL
                if (m_tokens.empty() || m_tokens.back().type != token_type::EOL) {
                    add_token(token_type::EOL);
                }

                new_line();
            }

            void handle_string() {
                // ignore opening quote
                m_start++;

                str_t value;

                while (peek() != '"' && peek() != '\n' && !is_end()) {
                    if (peek() == '\\') {
                        advance();  // consume backslash

                        // convert escape sequence to actual character
                        switch (peek()) {
                            case 'n':  value += '\n'; break;  // newline
                            case 't':  value += '\t'; break;  // tab
                            case 'r':  value += '\r'; break;  // carriage return
                            case '0':  value += '\0'; break;  // null
                            case '\\': value += '\\'; break;  // literal backslash
                            case '"':  value += '"';  break;  // literal quote
                            default:
                                // unrecognised escape - report but continue
                                report(error_type::INVALID_ESCAPE_CHAR);
                                return;
                        }
                        advance();  // consume the escape char
                    } else {
                        value += peek();
                        advance();
                    }
                }

                if (is_end() || peek() != '"') {
                    report(error_type::UNTERMINATED_STRING);
                    return;
                }

                advance();  // consume closing quote

                // value is already built - skip get_current_value() entirely
                add_token(token_type::STRING, value);
            }

            void handle_directive() {
                // ignore the DOT
                m_start++;

                while (is_almun(peek())) {
                    advance();
                }

                // get value
                str_t value = get_current_value();

                if(value.empty()) {
                    report(error_type::EXPECTED_DIRECTIVE);
                }
                else {
                    // NOTE: LC3 is case insensitive.
                    // Convert the token to uppercase only for directive lookup.
                    // This is used exclusively to check whether the token is a DIRECTIVE;
                    // the original token itself is not modified.
                    str_t upper = value;
                    // to uppercase
                    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);

                    // lookup
                    auto item = DIRECTIVES.find(upper);

                    // verify
                    if (item != DIRECTIVES.end()) {
                        add_token(item->second, value);
                    }
                    else {
                        report(error_type::INVALID_DIRECTIVE);
                    }
                }
            }

            void handle_dec_number(bool minus=false) {
                // ignore the `#`
                if (!minus) {
                    m_start++;
                }

                // if signed consume the sign (still saved in value)
                str_t value;

                // case: #-5
                if (peek() == '-') {
                    // consume and ignore the sign
                    advance();
                    m_start++;
                    value = "-";
                }

                while(is_digit(peek())) {
                    this->advance();
                }

                // get value
                value += get_current_value();

                add_token(token_type::DEC_NUMBER, value);
            }
            
            void handle_bare_decimal() {
                // no prefix to skip -- m_start already points at the first digit
                while (is_digit(peek())) {
                    advance();
                }

                str_t value = get_current_value();
                add_token(token_type::DEC_NUMBER, value);
            }

            void handle_identifier() {
                // NOTE: Read the entire lexeme first. Identifiers and hexadecimal literals both
                //       begin with alphabetic characters (e.g., 'x'), so they are treated
                //       identically while scanning. Once the complete lexeme is collected, it is
                //       classified as either a hexadecimal literal or an identifier based on its
                //       final contents. A lexeme is considered a hexadecimal literal only if it
                //       matches the hex literal format exactly; otherwise, it is classified as an
                //       identifier.

                while (is_almun(peek())) {
                    advance();
                }

                // get value
                // cannot be empty
                str_t value = get_current_value();
                str_t upper = value;
                
                // NOTE: LC-3 is case insensitive - labels, mnemonics, registers, everything.
                std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);

                auto item = MNEMONICS.find(upper);
                auto item_ext = MNEMONICS_EXT.end();

                if (is_ext_enabled()) {
                    item_ext = MNEMONICS_EXT.find(upper);
                }

                // check: if not a mnemonic, it can be a hex value, if not its an identifier (label, register)
                if (item != MNEMONICS.end()) {
                    add_token(item->second, upper);
                }
                // lc3kit-ext
                else if (item_ext != MNEMONICS_EXT.end()) {
                    add_token(item_ext->second, upper);
                }
                else if (is_hex(value)) {
                    add_token(token_type::HEX_NUMBER, value);                
                }
                else {
                    add_token(token_type::IDENTIFIER, value);                
                }
            }
            
        public:
            using BaseObj::BaseObj;

            void tokenize(source_code src) {
                if (!src.data || !src.size) {
                    report(error_type::INVALID_SOURCE_CODE);
                    return;
                }

                reset();
                // set source
                m_src = src;
                m_current_pos = {0, 0};
                m_current = 0;
                m_start = 0;
                m_tokens.clear();

                start();

                while (!is_end()) {
                    m_start = m_current;

                    handle_token();
                }

                add_token(token_type::EOF_);
                stop();
            }

            const tokens_t& tokens() const {
                return m_tokens;
            }

            const source_code& source() const {
                return m_src;
            }                    
        };


} // namespace lc3kit::lasm


#pragma once

#include "ds.h"

namespace lc3kit::lasm
{
    /**
     * @brief Error conditions that can occur during lexing, parsing, or encoding.
     */
    enum class error_type {
        NO_ERROR,

        // lexer's
        UNTERMINATED_STRING,
        UNEXPECTED_CHAR,
        EXPECTED_DIRECTIVE,
        INVALID_DIRECTIVE,
        INVALID_SOURCE_CODE,
        INVALID_ESCAPE_CHAR,

        // parser's
        INVALID_NUMBER,
        NUMBER_OUT_OF_RANGE,
        INVALID_SIGN,
        EXPECTED_REGISTER,
        INVALID_REGISTER_FORMAT,
        REGISTER_OUT_OF_RANGE,
        EXPECTED_EOL,
        EXPECTED_COMMA,
        EXPECTED_NUMBER,
        EXPECTED_STRING,
        UNEXPECTED_TOKEN,
        EXPECTED_LABEL_OR_NUMBER,

        // encoder's
        UNDEFINED_LABEL,
        NO_ORIG
    };

    /**
     * @brief Convert an assembler error code to a readable message.
     *
     * @param err Error code to describe.
     * @return Human-readable error text.
     */
    inline constexpr const char* err_str(error_type err) {
        switch (err) {
            case error_type::NO_ERROR:
                return "No error.";

            // Lexer
            case error_type::UNTERMINATED_STRING:
                return "Unterminated string literal.";

            case error_type::UNEXPECTED_CHAR:
                return "Unexpected character.";

            case error_type::EXPECTED_DIRECTIVE:
                return "Expected directive name after '.'.";

            case error_type::INVALID_DIRECTIVE:
                return "Invalid directive.";

            case error_type::INVALID_SOURCE_CODE:
                return "Invalid source code.";

            case error_type::INVALID_ESCAPE_CHAR:
                return "Invalid escape charachter.";

            // Parser
            case error_type::INVALID_NUMBER:
                return "Invalid numeric literal.";

            case error_type::NUMBER_OUT_OF_RANGE:
                return "Numeric literal is out of range.";

            case error_type::INVALID_SIGN:
                return "Invalid sign for numeric literal.";

            case error_type::EXPECTED_REGISTER:
                return "Expected register.";

            case error_type::INVALID_REGISTER_FORMAT:
                return "Invalid register format.";

            case error_type::REGISTER_OUT_OF_RANGE:
                return "Register index is out of range.";

            case error_type::EXPECTED_EOL:
                return "Expected end of line.";

            case error_type::EXPECTED_COMMA:
                return "Expected ','.";

            case error_type::EXPECTED_NUMBER:
                return "Expected numeric literal.";

            case error_type::EXPECTED_STRING:
                return "Expected string literal.";

            case error_type::UNEXPECTED_TOKEN:
                return "Unexpected token.";

            case error_type::EXPECTED_LABEL_OR_NUMBER:
                return "Expected label or numeric literal.";

            // Encoder
            case error_type::UNDEFINED_LABEL:
                return "Undefined label.";

            case error_type::NO_ORIG:
                return "Missing .ORIG directive.";
            }

        return "Unknown error.";
    }
    
    typedef struct Error {
        error_type type;
        tpos       pos;

        Error(error_type err = error_type::NO_ERROR, tpos p = {0, 0}) : type(err), pos(p) { }
    } error;

    using errors_t = std::vector<Error>;

} // namespace lc3kit::lasm

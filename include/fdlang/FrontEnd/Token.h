#pragma once
#include <cstdint>
#include <string_view>

namespace fl {
    enum class TokenType {
        END_OF_FILE,
        ERROR,
        
        // operators
        PLUS,                // +
        MINUS,               // -
        MULTIPLY,            // *
        DIVIDE,              // /
        MODULO,              // %
        EQUAL,               // =
        LESS_THAN,           // <
        GREATER_THAN,        // >

        EQUAL_EQUAL,         // ==
        LESS_THAN_EQUAL,     // <=
        GREATER_THAN_EQUAL,  // >=
        NOT_EQUAL,           // !=

        // --- CÁC DẤU CÂU CẤU TRÚC (Bắt buộc phải có theo Grammar) ---
        COLON,               // :
        COLON_COLON,         // :: (Rất quan trọng cho tính năng mod)
        SEMI,                // ;
        COMMA,               // ,
        L_PAREN,             // (
        R_PAREN,             // )
        L_BRACE,             // {
        R_BRACE,             // }

        // literals
        IDENTIFIER,
        NUMBER,
        STRING,

        // keywords
        KW_DEC,     // dec 
        KW_FN,      // fn   
        KW_IF,      // if   
        KW_ELSE,    // else   
        KW_RETURN,  // return
        KW_EXPORT,  // export
        KW_EXTERN,  // extern
        KW_CONST,   // const
        KW_MOD,     // mod
        KW_TRUE,    // true  
        KW_FALSE    // false
    };

    class Token {
    public:
        TokenType type;
        std::string_view text;
        uint32_t byteOffset; // offset in the source code
    };
}
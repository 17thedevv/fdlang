#pragma once
#include <string_view>
#include "Token.h"

namespace fl {

class Lexer {
private:
    std::string_view source; 
    uint32_t position;       

    // --- Các hàm công cụ thao tác con trỏ ---
    char peek() const;
    char peekNext() const;
    char peekNextNext() const;
    char advance();
    bool isAtEnd() const;
    bool match(char expected);

    // --- Bỏ qua khoảng trắng và comment ---
    void skipWhitespaceAndComments();

    // --- Khởi tạo Token ---
    Token makeToken(TokenType type, uint32_t startOffset, uint32_t length);
    Token errorToken(const char* message, uint32_t startOffset);

    // --- Rẽ nhánh xử lý ---
    Token identifierOrKeyword();
    Token numberLiteral();
    Token charLiteral(bool isByte);
    Token stringLiteral(bool isRaw, bool isByte);

public:
    explicit Lexer(std::string_view sourceCode);
    
    // Hàm cốt lõi: Parser sẽ gọi hàm này liên tục để lấy Token
    Token nextToken(); 
};

} // namespace fl
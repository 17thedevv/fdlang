#include "fdlang/Frontend/Lexer.h"
#include <cctype>

namespace fl {

Lexer::Lexer(std::string_view sourceCode) : source(sourceCode), position(0) {}

bool Lexer::isAtEnd() const {
    return position >= source.length();
}

char Lexer::peek() const {
    if (isAtEnd()) return '\0';
    return source[position];
}

char Lexer::peekNext() const {
    if (position + 1 >= source.length()) return '\0';
    return source[position + 1];
}

char Lexer::advance() {
    if (isAtEnd()) return '\0';
    return source[position++];
}

// Nếu ký tự hiện tại giống 'expected', tiến lên 1 bước và trả về true (Dùng cho ==, !=, <=)
bool Lexer::match(char expected) {
    if (isAtEnd() || source[position] != expected) return false;
    position++;
    return true;
}

Token Lexer::makeToken(TokenType type, uint32_t startOffset, uint32_t length) {
    Token token;
    token.type = type;
    token.text = source.substr(startOffset, length); // Trích xuất string_view, O(1) time
    token.byteOffset = startOffset;
    return token;
}

Token Lexer::errorToken(const char* message, uint32_t startOffset) {
    Token token;
    token.type = TokenType::ERROR;
    token.text = message; 
    token.byteOffset = startOffset;
    return token;
}

void Lexer::skipWhitespaceAndComments() {
    while (true) {
        char c = peek();
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
            case '\n':
                advance();
                break;
            case '/':
                if (peekNext() == '/') {
                    // Gặp comment "//", bỏ qua đến cuối dòng
                    while (peek() != '\n' && !isAtEnd()) advance();
                } else {
                    return; // Nó là dấu chia '/', không phải comment
                }
                break;
            default:
                return;
        }
    }
}

Token Lexer::identifierOrKeyword() {
    uint32_t startOffset = position;
    // Tên biến/hàm được bắt đầu bằng chữ cái, theo sau là chữ, số hoặc gạch dưới
    while (std::isalnum(peek()) || peek() == '_') {
        advance();
    }

    uint32_t length = position - startOffset;
    std::string_view text = source.substr(startOffset, length);

    // Kiểm tra xem nó có phải là từ khóa không
    TokenType type = TokenType::IDENTIFIER;
    if (text == "dec") type = TokenType::KW_DEC;
    else if (text == "fn") type = TokenType::KW_FN;
    else if (text == "if") type = TokenType::KW_IF;
    else if (text == "else") type = TokenType::KW_ELSE;
    else if (text == "return") type = TokenType::KW_RETURN;
    else if (text == "export") type = TokenType::KW_EXPORT;
    else if (text == "extern") type = TokenType::KW_EXTERN;
    else if (text == "const") type = TokenType::KW_CONST;
    else if (text == "mod") type = TokenType::KW_MOD;
    else if (text == "true") type = TokenType::KW_TRUE;
    else if (text == "false") type = TokenType::KW_FALSE;

    return makeToken(type, startOffset, length);
}

Token Lexer::number() {
    uint32_t startOffset = position;
    while (std::isdigit(peek())) {
        advance();
    }
    // Hỗ trợ số thập phân (ví dụ: 3.14)
    if (peek() == '.' && std::isdigit(peekNext())) {
        advance(); // Ăn dấu '.'
        while (std::isdigit(peek())) {
            advance();
        }
    }
    return makeToken(TokenType::NUMBER, startOffset, position - startOffset);
}

Token Lexer::string() {
    uint32_t startOffset = position;
    advance(); // Ăn dấu ngoặc kép mở '"'

    while (peek() != '"' && !isAtEnd()) {
        advance();
    }

    if (isAtEnd()) return errorToken("Unterminated string.", startOffset);

    advance(); // Ăn dấu ngoặc kép đóng '"'
    return makeToken(TokenType::STRING, startOffset, position - startOffset);
}

Token Lexer::nextToken() {
    skipWhitespaceAndComments();

    uint32_t startOffset = position;

    if (isAtEnd()) return makeToken(TokenType::END_OF_FILE, startOffset, 0);

    char c = advance();

    // Rẽ nhánh: Chữ cái hoặc dấu gạch dưới -> Biến/Từ khóa
    if (std::isalpha(c) || c == '_') {
        position--; // Lùi lại 1 bước để hàm identifier tự tiêu thụ ký tự đầu tiên
        return identifierOrKeyword();
    }
    // Rẽ nhánh: Số -> Number Literal
    if (std::isdigit(c)) {
        position--;
        return number();
    }

    // Rẽ nhánh: Các dấu câu và toán tử
    switch (c) {
        case '+': return makeToken(TokenType::PLUS, startOffset, 1);
        case '-': return makeToken(TokenType::MINUS, startOffset, 1);
        case '*': return makeToken(TokenType::MULTIPLY, startOffset, 1);
        case '/': return makeToken(TokenType::DIVIDE, startOffset, 1);
        case '%': return makeToken(TokenType::MODULO, startOffset, 1);
        case ';': return makeToken(TokenType::SEMI, startOffset, 1);
        case ',': return makeToken(TokenType::COMMA, startOffset, 1);
        case '(': return makeToken(TokenType::L_PAREN, startOffset, 1);
        case ')': return makeToken(TokenType::R_PAREN, startOffset, 1);
        case '{': return makeToken(TokenType::L_BRACE, startOffset, 1);
        case '}': return makeToken(TokenType::R_BRACE, startOffset, 1);
        
        // Toán tử 1 hoặc 2 ký tự
        case '=':
            return match('=') ? makeToken(TokenType::EQUAL_EQUAL, startOffset, 2)
                              : makeToken(TokenType::EQUAL, startOffset, 1);
        case '<':
            return match('=') ? makeToken(TokenType::LESS_THAN_EQUAL, startOffset, 2)
                              : makeToken(TokenType::LESS_THAN, startOffset, 1);
        case '>':
            return match('=') ? makeToken(TokenType::GREATER_THAN_EQUAL, startOffset, 2)
                              : makeToken(TokenType::GREATER_THAN, startOffset, 1);
        case '!':
            return match('=') ? makeToken(TokenType::NOT_EQUAL, startOffset, 2)
                              : errorToken("Loi: Dau '!' phai di kem voi '='", startOffset);
        case ':':
            return match(':') ? makeToken(TokenType::COLON_COLON, startOffset, 2)
                              : makeToken(TokenType::COLON, startOffset, 1);
        case '"':
            position--; 
            return string();
    }

    return errorToken("Loi: Ky tu khong hop le.", startOffset);
}

} // namespace fl
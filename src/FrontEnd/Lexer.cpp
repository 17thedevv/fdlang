#include "fdlang/FrontEnd/Lexer.h"
#include <cctype>
#include <string>

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

char Lexer::peekNextNext() const {
    if (position + 2 >= source.length()) return '\0';
    return source[position + 2];
}

char Lexer::advance() {
    if (isAtEnd()) return '\0';
    return source[position++];
}

bool Lexer::match(char expected) {
    if (isAtEnd() || source[position] != expected) return false;
    position++;
    return true;
}

Token Lexer::makeToken(TokenType type, uint32_t startOffset, uint32_t length) {
    Token token;
    token.type = type;
    token.text = source.substr(startOffset, length);
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
                    while (peek() != '\n' && !isAtEnd()) advance();
                } else if (peekNext() == '*') {
                    advance(); // '/'
                    advance(); // '*'
                    while (!isAtEnd()) {
                        if (peek() == '*' && peekNext() == '/') {
                            advance(); // '*'
                            advance(); // '/'
                            break;
                        }
                        advance();
                    }
                } else {
                    return; 
                }
                break;
            default:
                return;
        }
    }
}

static bool isAlphaOrUnderscore(char c) {
    return std::isalpha(c) || c == '_';
}

static bool isAlnumOrUnderscore(char c) {
    return std::isalnum(c) || c == '_';
}

Token Lexer::identifierOrKeyword() {
    uint32_t startOffset = position;
    while (isAlnumOrUnderscore(peek())) {
        advance();
    }

    uint32_t length = position - startOffset;
    std::string_view text = source.substr(startOffset, length);

    TokenType type = TokenType::IDENTIFIER;
    if (text == "dec") type = TokenType::KW_DEC;
    else if (text == "const") type = TokenType::KW_CONST;
    else if (text == "fn") type = TokenType::KW_FN;
    else if (text == "return") type = TokenType::KW_RETURN;
    else if (text == "if") type = TokenType::KW_IF;
    else if (text == "else") type = TokenType::KW_ELSE;
    else if (text == "while") type = TokenType::KW_WHILE;
    else if (text == "for") type = TokenType::KW_FOR;
    else if (text == "in") type = TokenType::KW_IN;
    else if (text == "break") type = TokenType::KW_BREAK;
    else if (text == "continue") type = TokenType::KW_CONTINUE;
    else if (text == "mod") type = TokenType::KW_MOD;
    else if (text == "export") type = TokenType::KW_EXPORT;
    else if (text == "extern") type = TokenType::KW_EXTERN;
    else if (text == "struct") type = TokenType::KW_STRUCT;
    else if (text == "enum") type = TokenType::KW_ENUM;
    else if (text == "trait") type = TokenType::KW_TRAIT;
    else if (text == "impl") type = TokenType::KW_IMPL;
    else if (text == "unsafe") type = TokenType::KW_UNSAFE;
    else if (text == "use") type = TokenType::KW_USE;
    else if (text == "as") type = TokenType::KW_AS;
    else if (text == "match") type = TokenType::KW_MATCH;
    else if (text == "rw") type = TokenType::KW_RW;
    else if (text == "true") type = TokenType::KW_TRUE;
    else if (text == "false") type = TokenType::KW_FALSE;
    else if (text == "type") type = TokenType::KW_TYPE;
    else if (text == "sizeof") type = TokenType::KW_SIZEOF;
    else if (text == "alignof") type = TokenType::KW_ALIGNOF;
    else if (text == "await") type = TokenType::KW_AWAIT;
    else if (text == "async") type = TokenType::KW_ASYNC;
    else if (text == "comptime") type = TokenType::KW_COMPTIME;
    else if (text == "dyn") type = TokenType::KW_DYN;
    else if (text == "self") type = TokenType::KW_SELF_VAL;
    else if (text == "Self") type = TokenType::KW_SELF_TYP;
    else if (text == "print") type = TokenType::KW_PRINT;

    return makeToken(type, startOffset, length);
}

Token Lexer::numberLiteral() {
    uint32_t startOffset = position;

    if (peek() == '0') {
        char next = peekNext();
        if (next == 'x' || next == 'X' || next == 'o' || next == 'O' || next == 'b' || next == 'B') {
            advance(); 
            advance(); 
            while (std::isalnum(peek()) || peek() == '_') {
                advance();
            }
            return makeToken(TokenType::INTEGER_LITERAL, startOffset, position - startOffset);
        }
    }

    while (std::isdigit(peek()) || peek() == '_') {
        advance();
    }

    bool isFloat = false;

    if (peek() == '.' && std::isdigit(peekNext())) {
        isFloat = true;
        advance(); 
        while (std::isdigit(peek()) || peek() == '_') {
            advance();
        }
    }

    if (peek() == 'e' || peek() == 'E') {
        isFloat = true;
        advance(); 
        if (peek() == '+' || peek() == '-') advance();
        while (std::isdigit(peek()) || peek() == '_') {
            advance();
        }
    }

    while (std::isalnum(peek()) || peek() == '_') {
        advance();
    }

    TokenType type = isFloat ? TokenType::FLOAT_LITERAL : TokenType::INTEGER_LITERAL;
    return makeToken(type, startOffset, position - startOffset);
}

Token Lexer::charLiteral(bool isByte) {
    uint32_t startOffset = position;
    if (isByte) advance(); // 'b'
    advance(); // '\''

    while (peek() != '\'' && !isAtEnd()) {
        if (peek() == '\\') {
            advance(); // escape
        }
        advance();
    }

    if (isAtEnd()) return errorToken("Unterminated char literal.", startOffset);

    advance(); // '\''
    
    TokenType type = isByte ? TokenType::BYTE_LITERAL : TokenType::CHAR_LITERAL;
    return makeToken(type, startOffset, position - startOffset);
}

Token Lexer::stringLiteral(bool isRaw, bool isByte) {
    uint32_t startOffset = position;
    if (isByte) advance(); 
    if (isRaw) advance();  
    
    int poundCount = 0;
    if (isRaw) {
        while (peek() == '#') {
            poundCount++;
            advance();
        }
    }

    advance(); // '"'

    while (!isAtEnd()) {
        if (!isRaw && peek() == '"') {
            break;
        } else if (!isRaw && peek() == '\\') {
            advance();
            advance();
            continue;
        } else if (isRaw && peek() == '"') {
            bool matchPounds = true;
            uint32_t lookahead = 1;
            for (int i = 0; i < poundCount; i++) {
                if (position + lookahead >= source.length() || source[position + lookahead] != '#') {
                    matchPounds = false;
                    break;
                }
                lookahead++;
            }
            if (matchPounds) {
                break;
            }
        }
        advance();
    }

    if (isAtEnd()) return errorToken("Unterminated string literal.", startOffset);

    advance(); 
    if (isRaw) {
        for (int i = 0; i < poundCount; i++) advance(); 
    }

    TokenType type = TokenType::STRING_LITERAL;
    if (isRaw) type = TokenType::RAW_STRING_LITERAL;
    if (isByte && !isRaw) type = TokenType::BYTE_STRING_LITERAL;
    
    return makeToken(type, startOffset, position - startOffset);
}

Token Lexer::nextToken() {
    skipWhitespaceAndComments();
    uint32_t startOffset = position;

    if (isAtEnd()) return makeToken(TokenType::END_OF_FILE, startOffset, 0);

    char c = advance();

    if (isAlphaOrUnderscore(c)) {
        if (c == 'r' && (peek() == '"' || peek() == '#')) {
            position--;
            return stringLiteral(true, false);
        }
        if (c == 'b') {
            if (peek() == '\'') {
                position--;
                return charLiteral(true);
            } else if (peek() == '"') {
                position--;
                return stringLiteral(false, true);
            }
        }
        position--;
        return identifierOrKeyword();
    }

    if (std::isdigit(c)) {
        position--;
        return numberLiteral();
    }

    switch (c) {
        case '+':
            if (match('=')) return makeToken(TokenType::PLUS_ASSIGN, startOffset, 2);
            if (match('+')) return makeToken(TokenType::PLUS_PLUS, startOffset, 2);
            return makeToken(TokenType::PLUS, startOffset, 1);
        case '-':
            if (match('=')) return makeToken(TokenType::MINUS_ASSIGN, startOffset, 2);
            if (match('-')) return makeToken(TokenType::MINUS_MINUS, startOffset, 2);
            if (match('>')) return makeToken(TokenType::ARROW, startOffset, 2);
            return makeToken(TokenType::MINUS, startOffset, 1);
        case '*':
            if (match('=')) return makeToken(TokenType::STAR_ASSIGN, startOffset, 2);
            return makeToken(TokenType::MULTIPLY, startOffset, 1);
        case '/':
            if (match('=')) return makeToken(TokenType::SLASH_ASSIGN, startOffset, 2);
            return makeToken(TokenType::DIVIDE, startOffset, 1);
        case '%':
            if (match('=')) return makeToken(TokenType::PERC_ASSIGN, startOffset, 2);
            return makeToken(TokenType::MODULO, startOffset, 1);
        case '=':
            if (match('=')) return makeToken(TokenType::EQUAL_EQUAL, startOffset, 2);
            return makeToken(TokenType::EQUAL, startOffset, 1);
        case '<':
            if (match('=')) return makeToken(TokenType::LESS_THAN_EQUAL, startOffset, 2);
            if (match('<')) {
                if (match('=')) return makeToken(TokenType::LSHIFT_ASSIGN, startOffset, 3);
                return makeToken(TokenType::LSHIFT, startOffset, 2);
            }
            return makeToken(TokenType::LESS_THAN, startOffset, 1);
        case '>':
            if (match('=')) return makeToken(TokenType::GREATER_THAN_EQUAL, startOffset, 2);
            if (match('>')) {
                if (match('=')) return makeToken(TokenType::RSHIFT_ASSIGN, startOffset, 3);
                return makeToken(TokenType::RSHIFT, startOffset, 2);
            }
            return makeToken(TokenType::GREATER_THAN, startOffset, 1);
        case '!':
            if (match('=')) return makeToken(TokenType::NOT_EQUAL, startOffset, 2);
            return makeToken(TokenType::BANG, startOffset, 1);
        case '&':
            if (match('=')) return makeToken(TokenType::BIT_AND_ASSIGN, startOffset, 2);
            if (match('&')) return makeToken(TokenType::LOGICAL_AND, startOffset, 2);
            return makeToken(TokenType::BIT_AND, startOffset, 1);
        case '|':
            if (match('=')) return makeToken(TokenType::BIT_OR_ASSIGN, startOffset, 2);
            if (match('|')) return makeToken(TokenType::LOGICAL_OR, startOffset, 2);
            return makeToken(TokenType::BIT_OR, startOffset, 1);
        case '^':
            if (match('=')) return makeToken(TokenType::BIT_XOR_ASSIGN, startOffset, 2);
            return makeToken(TokenType::BIT_XOR, startOffset, 1);
        case '~': return makeToken(TokenType::BIT_NOT, startOffset, 1);
        case '?': return makeToken(TokenType::QUESTION, startOffset, 1);
        case ':':
            if (match(':')) return makeToken(TokenType::COLON_COLON, startOffset, 2);
            return makeToken(TokenType::COLON, startOffset, 1);
        case '.':
            if (match('.')) {
                if (match('.')) return makeToken(TokenType::DOT_DOT_DOT, startOffset, 3);
                if (match('=')) return makeToken(TokenType::DOT_DOT_EQ, startOffset, 3);
                return makeToken(TokenType::DOT_DOT, startOffset, 2);
            }
            return makeToken(TokenType::DOT, startOffset, 1);
        case '@':
            if (match('[')) return makeToken(TokenType::AT_BRACKET, startOffset, 2);
            if (match('<')) return makeToken(TokenType::GENERIC_START, startOffset, 2);
            return errorToken("Invalid '@'", startOffset);
        case ';': return makeToken(TokenType::SEMI, startOffset, 1);
        case ',': return makeToken(TokenType::COMMA, startOffset, 1);
        case '(': return makeToken(TokenType::L_PAREN, startOffset, 1);
        case ')': return makeToken(TokenType::R_PAREN, startOffset, 1);
        case '{': return makeToken(TokenType::L_BRACE, startOffset, 1);
        case '}': return makeToken(TokenType::R_BRACE, startOffset, 1);
        case '[': return makeToken(TokenType::L_BRACKET, startOffset, 1);
        case ']': return makeToken(TokenType::R_BRACKET, startOffset, 1);
        case '\'':
            position--;
            return charLiteral(false);
        case '"':
            position--;
            return stringLiteral(false, false);
    }

    return errorToken("Invalid character.", startOffset);
}

} // namespace fl
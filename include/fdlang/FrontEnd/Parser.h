#pragma once
#include "fdlang/Frontend/Lexer.h"
#include "fdlang/AST/ASTNode.h"
#include "fdlang/AST/ExprNode.h"
#include "fdlang/AST/StmtNode.h"
#include <memory>
#include <string_view>

namespace fl {

class Parser {
private:
    Lexer& lexer;      // Tham chiếu đến cỗ máy Lexer (Pull-based)
    Token current;     // Token đang đứng hiện tại
    Token peek;        // Token đi trước 1 bước (để nhìn trộm, giúp rẽ nhánh)

    // --- Các hàm thao tác con trỏ Token ---
    void advance();
    bool check(TokenType type) const;
    bool match(TokenType type);
    Token consume(TokenType type, const char* errorMessage);

    // Program
std::unique_ptr<StmtNode> parseDeclaration();
std::unique_ptr<StmtNode> parseStatement();

// Declaration
std::unique_ptr<StmtNode> parseVarDeclaration();

// Statements
std::unique_ptr<StmtNode> parseAssignmentStatement();
std::unique_ptr<StmtNode> parseExpressionStatement();
std::unique_ptr<StmtNode> parseBlockStatement();
std::unique_ptr<StmtNode> parseIfStatement();
std::unique_ptr<StmtNode> parseWhileStatement();
std::unique_ptr<StmtNode> parsePrintStatement();

// Expressions
std::unique_ptr<ExprNode> parseExpression();
std::unique_ptr<ExprNode> parseEquality();
std::unique_ptr<ExprNode> parseComparison();
std::unique_ptr<ExprNode> parseTerm();
std::unique_ptr<ExprNode> parseFactor();
std::unique_ptr<ExprNode> parseUnary();
std::unique_ptr<ExprNode> parsePostfix();
std::unique_ptr<ExprNode> parsePrimary();
public:
    explicit Parser(Lexer& lexer);
    
    // Hàm khởi chạy toàn bộ quá trình phân tích
    std::unique_ptr<ProgramNode> parse();
};

} // namespace fl
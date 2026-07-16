#include "fdlang/Frontend/Parser.h"
#include "fdlang/Frontend/ASTVisitor.h"
#include <iostream>
#include <cstdlib>

namespace fl {

Parser::Parser(Lexer& lexer) : lexer(lexer) {
    // Khởi động mồi 2 nhịp để nạp đầy 'current' và 'peek'
    advance();
    advance();
}

void Parser::advance() {
    current = peek;
    peek = lexer.nextToken();
}

bool Parser::check(TokenType type) const {
    if (current.type == TokenType::END_OF_FILE) return false;
    return current.type == type;
}

// Nếu đúng loại Token cần tìm, tiến lên 1 bước và báo thành công
bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

// Hàm cực kỳ quan trọng: "Bắt buộc phải có Token này, nếu không thì báo lỗi ngừng biên dịch"
Token Parser::consume(TokenType type, const char* errorMessage) {
    if (check(type)) {
        Token t = current;
        advance();
        return t;
    }
    
    // Nếu cú pháp sai (ví dụ thiếu dấu ;), báo lỗi và dừng ngay lập tức
    std::cerr << "Loi cu phap tai offset " << current.byteOffset << ": " << errorMessage << "\n";
    std::exit(65); // EX_DATAERR
}

// ==========================================
// CÁC HÀM XÂY DỰNG CÂY AST
// ==========================================

std::unique_ptr<ProgramNode> Parser::parse() {
    auto program = std::make_unique<ProgramNode>();

    while (current.type != TokenType::END_OF_FILE) {
        program->statements.push_back(parseDeclaration());
    }

    return program;
}

std::unique_ptr<StmtNode> Parser::parseDeclaration() {

    // ===== Declarations =====

    if (match(TokenType::KW_DEC))
        return parseVarDeclaration();

    // Future
    //
    // if(match(TokenType::KW_CONST))
    //     return parseConstDeclaration();
    //
    // if(match(TokenType::KW_FN))
    //     return parseFunctionDeclaration();
    //
    // if(match(TokenType::KW_EXTERN))
    //     ...
    //
    // if(match(TokenType::KW_EXPORT))
    //     ...
    //
    // if(match(TokenType::KW_MOD))
    //     ...

    return parseStatement();
}

std::unique_ptr<StmtNode> Parser::parseStatement() {

    switch (current.type) {

    case TokenType::KW_PRINT:
        return parsePrintStatement();

    case TokenType::KW_IF:
        return parseIfStatement();

    case TokenType::KW_WHILE:
        return parseWhileStatement();

    case TokenType::L_BRACE:
        return parseBlockStatement();

    case TokenType::IDENTIFIER:
        return parseAssignmentStatement();

    default:
        std::cerr
            << "Loi: Cau lenh khong hop le tai offset "
            << current.byteOffset
            << "\n";

        std::exit(65);
    }
}

// --- CÁC HÀM PHÂN TÍCH CÂU LỆNH (STATEMENTS) ---

std::unique_ptr<StmtNode> Parser::parseVarDeclaration() {
    Token nameToken = consume(TokenType::IDENTIFIER, "Thieu ten bien sau 'dec'.");
    
    std::unique_ptr<ExprNode> initializer = nullptr;

    if (match(TokenType::EQUAL)) {
        initializer = parseExpression();
    }
    
    consume(TokenType::SEMI, "Thieu dau ';' o cuoi cau lenh.");
    return std::make_unique<VarDeclStmt>(nameToken.text, std::move(initializer));
}

std::unique_ptr<StmtNode> Parser::parseAssignmentStatement() {
    Token nameToken = consume(TokenType::IDENTIFIER, "Thieu ten bien trong lenh gan.");
    consume(TokenType::EQUAL, "Thieu dau '=' trong lenh gan.");
    
    auto value = parseExpression();
    consume(TokenType::SEMI, "Thieu dau ';' sau lenh gan.");
    
    return std::make_unique<AssignStmtNode>(nameToken.text, std::move(value));
}

std::unique_ptr<StmtNode> Parser::parseExpressionStatement() {

    auto expr = parseExpression();

    consume(TokenType::SEMI,
            "Thieu ';' sau bieu thuc.");

    // TODO:
    // return std::make_unique<ExpressionStmtNode>(std::move(expr));

    std::cerr << "Expression statement chua duoc ho tro.\n";
    std::exit(65);
}

std::unique_ptr<StmtNode> Parser::parsePrintStatement() {
    advance(); // Ăn chữ 'print'
    auto value = parseExpression();
    consume(TokenType::SEMI, "Thieu dau ';' sau lenh print.");
    return std::make_unique<PrintStmtNode>(std::move(value));
}

std::unique_ptr<StmtNode> Parser::parseBlockStatement() {
    advance(); // Ăn dấu '{'
    std::vector<std::unique_ptr<StmtNode>> statements;
    
    while (!check(TokenType::R_BRACE) && !check(TokenType::END_OF_FILE)) {
        statements.push_back(parseDeclaration());
    }
    
    consume(TokenType::R_BRACE, "Thieu dau '}' de ket thuc khoi lenh.");
    return std::make_unique<BlockStmtNode>(std::move(statements));
}

std::unique_ptr<StmtNode> Parser::parseIfStatement() {
    advance(); // Ăn chữ 'if'
    consume(TokenType::L_PAREN, "Thieu dau '(' sau 'if'.");
    auto condition = parseExpression();
    consume(TokenType::R_PAREN, "Thieu dau ')' sau dieu kien 'if'.");

    auto thenBranch = parseDeclaration();
    
    std::unique_ptr<StmtNode> elseBranch = nullptr;

    if (match(TokenType::KW_ELSE)) {
        elseBranch = parseDeclaration();
    }

    return std::make_unique<IfStmtNode>(std::move(condition), std::move(thenBranch), std::move(elseBranch));
}

std::unique_ptr<StmtNode> Parser::parseWhileStatement() {
    advance(); // Ăn chữ 'while'
    consume(TokenType::L_PAREN, "Thieu dau '(' sau 'while'.");
    auto condition = parseExpression();
    consume(TokenType::R_PAREN, "Thieu dau ')' sau dieu kien 'while'.");
    
    auto body = parseStatement();
    return std::make_unique<WhileStmtNode>(std::move(condition), std::move(body));
}

// --- CÁC HÀM PHÂN TÍCH BIỂU THỨC (EXPRESSIONS - RECURSIVE DESCENT) ---

std::unique_ptr<ExprNode> Parser::parseExpression() {
    // Điểm bắt đầu của chuỗi ưu tiên
    return parseEquality();
}

std::unique_ptr<ExprNode> Parser::parseEquality() {
    auto expr = parseComparison();

    while (check(TokenType::EQUAL_EQUAL) || check(TokenType::NOT_EQUAL)) {
        Token op = current; 
        advance();
        auto right = parseComparison();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op.text, std::move(right));
    }
    return expr;
}

std::unique_ptr<ExprNode> Parser::parseComparison() {
    auto expr = parseTerm();

    while (check(TokenType::LESS_THAN) || check(TokenType::LESS_THAN_EQUAL) || 
           check(TokenType::GREATER_THAN) || check(TokenType::GREATER_THAN_EQUAL)) {
        Token op = current; 
        advance();
        auto right = parseTerm();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op.text, std::move(right));
    }
    return expr;
}

std::unique_ptr<ExprNode> Parser::parseTerm() {
    auto expr = parseFactor();

    while (check(TokenType::PLUS) || check(TokenType::MINUS)) {
        Token op = current; 
        advance();
        auto right = parseFactor();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op.text, std::move(right));
    }
    return expr;
}

std::unique_ptr<ExprNode> Parser::parseFactor() {
    auto expr = parseUnary();

    while (check(TokenType::MULTIPLY)
        || check(TokenType::DIVIDE)
        || check(TokenType::MODULO))
    {
        Token op = current;
        advance();

        auto right = parseUnary();

        expr = std::make_unique<BinaryExpr>(
            std::move(expr),
            op.text,
            std::move(right)
        );
    }

    return expr;
}

std::unique_ptr<ExprNode> Parser::parsePrimary() {
    if (match(TokenType::KW_TRUE)) return std::make_unique<BooleanExpr>(true);
    if (match(TokenType::KW_FALSE)) return std::make_unique<BooleanExpr>(false);
    
    if (check(TokenType::NUMBER)) {
        Token num = current; 
        advance();
        return std::make_unique<NumberExpr>(num.text);
    }
    
    if (check(TokenType::IDENTIFIER)) {
        Token id = current; 
        advance();
        return std::make_unique<IdentifierExpr>(id.text);
    }
    
    if (match(TokenType::L_PAREN)) {
        auto expr = parseExpression();
        consume(TokenType::R_PAREN, "Thieu dau ')' dong ngoac bieu thuc.");
        return expr;
    }
    
    std::cerr << "Loi: Bieu thuc khong hop le tai offset " << current.byteOffset << "\n";
    std::exit(65);
}

std::unique_ptr<ExprNode> Parser::parseUnary() {

    // Future:
    //
    // if(match(TokenType::NOT))
    // if(match(TokenType::MINUS))
    // if(match(TokenType::AMPERSAND))
    // if(match(TokenType::STAR))
    //

    return parsePostfix();
}

std::unique_ptr<ExprNode> Parser::parsePostfix() {

    auto expr = parsePrimary();

    // Future:
    //
    // foo()
    // obj.field
    // arr[index]
    // Math::add()

    return expr;
}

void ProgramNode::accept(ASTVisitor* visitor) {
    visitor->visit(this);
}

} // namespace fl
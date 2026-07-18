import re

with open('src/FrontEnd/Parser.cpp', 'r') as f:
    content = f.read()

# Replace parseForStatement
replacement = r'''std::unique_ptr<StmtNode> Parser::parseForStatement() {
    auto forStmt = std::make_unique<ForStmtNode>();
    forStmt->loc = SourceLocation{current.byteOffset, 0, 0};
    consume(TokenType::KW_FOR, "Expected 'for'");
    consume(TokenType::L_PAREN, "Expected '(' after 'for'");

    if (current.type == TokenType::KW_DEC || current.type == TokenType::KW_CONST || current.type == TokenType::SEMI) {
        forStmt->kind = ForKind::CStyle;
        if (current.type != TokenType::SEMI) {
            forStmt->init = parseDeclaration();
        } else {
            advance();
        }
        
        if (current.type != TokenType::SEMI) {
            forStmt->cond = parseExpression(false);
        }
        consume(TokenType::SEMI, "Expected ';' after for condition");
        
        if (current.type != TokenType::R_PAREN) {
            forStmt->step = parseExpression(false);
        }
        consume(TokenType::R_PAREN, "Expected ')' after for update");
    } else {
        Token ident = consume(TokenType::IDENTIFIER, "Expected loop variable");
        forStmt->bindingName = ident.text;
        consume(TokenType::KW_IN, "Expected 'in' after loop variable");
        forStmt->iterable = parseExpression(true);
        consume(TokenType::R_PAREN, "Expected ')' after for condition");
        forStmt->kind = ForKind::ForEach;
    }

    forStmt->body = parseBlockStatement();
    return forStmt;
}'''

content = re.sub(
    r'std::unique_ptr<StmtNode> Parser::parseForStatement\(\) \{.*?return forStmt;\n\}',
    replacement,
    content,
    flags=re.DOTALL
)

with open('src/FrontEnd/Parser.cpp', 'w') as f:
    f.write(content)

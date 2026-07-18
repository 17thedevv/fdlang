import re

with open('src/FrontEnd/Parser.cpp', 'r') as f:
    content = f.read()

# 1. comptime parsing fix
content = re.sub(
    r'    std::unique_ptr<DeclNode> decl = nullptr;\s*if \(current\.type == TokenType::KW_DEC \|\| current\.type == TokenType::KW_CONST\) \{',
    r'''    std::unique_ptr<DeclNode> decl = nullptr;
    
    bool isFunction = false;
    if (current.type == TokenType::KW_FN || current.type == TokenType::KW_ASYNC) {
        isFunction = true;
    } else if (current.type == TokenType::KW_COMPTIME) {
        if (peek.type == TokenType::KW_FN || peek.type == TokenType::KW_ASYNC) {
            isFunction = true;
        }
    }

    if (current.type == TokenType::KW_DEC || current.type == TokenType::KW_CONST) {''',
    content
)

content = re.sub(
    r'    \} else if \(current\.type == TokenType::KW_FN \|\| current\.type == TokenType::KW_ASYNC \|\| current\.type == TokenType::KW_COMPTIME\) \{',
    r'    } else if (isFunction) {',
    content
)

# 2. Postfix ++ and --
content = re.sub(
    r'        if \(match\(TokenType::L_PAREN\)\) \{',
    r'''        if (match(TokenType::PLUS_PLUS)) {
            auto unExpr = std::make_unique<UnaryExpr>();
            unExpr->loc = SourceLocation{current.byteOffset, 0, 0};
            unExpr->op = UnaryOp::PostInc;
            unExpr->operand = std::move(expr);
            expr = std::move(unExpr);
            continue;
        }
        if (match(TokenType::MINUS_MINUS)) {
            auto unExpr = std::make_unique<UnaryExpr>();
            unExpr->loc = SourceLocation{current.byteOffset, 0, 0};
            unExpr->op = UnaryOp::PostDec;
            unExpr->operand = std::move(expr);
            expr = std::move(unExpr);
            continue;
        }
        if (match(TokenType::L_PAREN)) {''',
    content
)

with open('src/FrontEnd/Parser.cpp', 'w') as f:
    f.write(content)

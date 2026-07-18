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

# 2. implicit return
content = re.sub(
    r'std::unique_ptr<StmtNode> Parser::parseExpressionStatement\(\) \{\s*auto expr = parseExpression\(\);\s*consume\(TokenType::SEMI, "Expected \';\' after expression"\);',
    r'''std::unique_ptr<StmtNode> Parser::parseExpressionStatement() {
    auto expr = parseExpression();
    if (current.type == TokenType::R_BRACE) {
        auto stmt = std::make_unique<ExprStmtNode>();
        stmt->expr = std::move(expr);
        return stmt;
    }
    consume(TokenType::SEMI, "Expected ';' after expression");''',
    content
)

# 3. for loop
content = re.sub(
    r'std::unique_ptr<StmtNode> Parser::parseForStatement\(\) \{.*?return forStmt;\s*\}',
    r'''std::unique_ptr<StmtNode> Parser::parseForStatement() {
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
}''',
    content,
    flags=re.DOTALL
)

# 4. postfix ops and .await and ?
content = re.sub(
    r'std::unique_ptr<ExprNode> Parser::parsePostfix\(bool allowStructLiteral\) \{\s*auto expr = parsePrimary\(allowStructLiteral\);\s*while \(true\) \{\s*if \(match\(TokenType::L_PAREN\)\) \{',
    r'''std::unique_ptr<ExprNode> Parser::parsePostfix(bool allowStructLiteral) {
    auto expr = parsePrimary(allowStructLiteral);
    
    while (true) {
        if (match(TokenType::QUESTION)) {
            auto tryCall = std::make_unique<MethodCallExpr>();
            tryCall->loc = SourceLocation{current.byteOffset, 0, 0};
            tryCall->object = std::move(expr);
            tryCall->methodName = "__try";
            expr = std::move(tryCall);
            continue;
        }
        if (match(TokenType::PLUS_PLUS)) {
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

content = re.sub(
    r'\} else if \(match\(TokenType::DOT\)\) \{\s*// Member Access or Method Call\s*Token memberTok = consume\(TokenType::IDENTIFIER, "Expected member name"\);',
    r'''} else if (match(TokenType::DOT)) {
            if (match(TokenType::KW_AWAIT)) {
                auto awaitExpr = std::make_unique<AwaitExpr>();
                awaitExpr->loc = SourceLocation{current.byteOffset, 0, 0};
                awaitExpr->expr = std::move(expr);
                expr = std::move(awaitExpr);
                continue;
            }
            // Member Access or Method Call
            Token memberTok;
            if (current.type == TokenType::IDENTIFIER || current.type == TokenType::KW_PRINT) {
                memberTok = current;
                advance();
            } else {
                diag.error(SourceLocation{current.byteOffset, 0, 0}, "Expected member name");
                throw ParseError();
            }''',
    content
)

# 5. parseValuePath
content = re.sub(
    r'        // Support starting with \'self\' \(for value path\)\s*if \(match\(TokenType::KW_SELF_VAL\)\) \{\s*ident->segments\.push_back\("self"\);\s*\} else \{\s*Token first = consume\(TokenType::IDENTIFIER, "Expected identifier in path"\);\s*ident->segments\.push_back\(first\.text\);\s*\}',
    r'''        // Support starting with 'self' (for value path)
        if (match(TokenType::KW_SELF_VAL)) {
            ident->segments.push_back("self");
        } else if (match(TokenType::KW_SELF_TYP)) {
            ident->segments.push_back("Self");
        } else {
            Token first;
            if (current.type == TokenType::IDENTIFIER || current.type == TokenType::KW_PRINT) {
                first = current;
                advance();
            } else {
                diag.error(SourceLocation{current.byteOffset, 0, 0}, "Expected identifier in path");
                throw ParseError();
            }
            ident->segments.push_back(first.text);
        }''',
    content
)

# 6. parseNamedType
content = re.sub(
    r'    do \{\s*Token idTok = consume\(TokenType::IDENTIFIER, "Expected identifier in path"\);\s*node->path\.push_back\(idTok\.text\);',
    r'''    do {
        Token idTok;
        if (match(TokenType::KW_SELF_TYP)) {
            idTok = prev;
        } else if (current.type == TokenType::IDENTIFIER || current.type == TokenType::KW_PRINT) {
            idTok = current;
            advance();
        } else {
            diag.error(SourceLocation{current.byteOffset, 0, 0}, "Expected identifier or Self in type path");
            throw ParseError();
        }
        node->path.push_back(idTok.text);''',
    content
)

# 7. parseUseTree
content = re.sub(
    r'node\.segments\.push_back\(consume\(TokenType::IDENTIFIER, ""\)\.text\);',
    r'if (current.type == TokenType::IDENTIFIER || current.type == TokenType::KW_PRINT) { node.segments.push_back(current.text); advance(); } else { diag.error(SourceLocation{current.byteOffset, 0, 0}, "Expected identifier"); throw ParseError(); }',
    content
)
content = re.sub(
    r'node\.segments\.push_back\(consume\(TokenType::IDENTIFIER, "Expected identifier in path"\)\.text\);',
    r'if (current.type == TokenType::IDENTIFIER || current.type == TokenType::KW_PRINT) { node.segments.push_back(current.text); advance(); } else { diag.error(SourceLocation{current.byteOffset, 0, 0}, "Expected identifier in path"); throw ParseError(); }',
    content
)

with open('src/FrontEnd/Parser.cpp', 'w') as f:
    f.write(content)

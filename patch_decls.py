import re

with open('src/FrontEnd/Parser.cpp', 'r') as f:
    content = f.read()

# 1. Add parsing functions
new_functions = r'''
std::vector<AnnotationNode> Parser::parseAnnotations() {
    std::vector<AnnotationNode> annotations;
    while (match(TokenType::AT)) {
        consume(TokenType::L_BRACKET, "Expected '[' after '@'");
        do {
            AnnotationNode annot;
            annot.loc = SourceLocation::fromOffset(current.byteOffset);
            Token nameTok = consume(TokenType::IDENTIFIER, "Expected annotation name");
            annot.name = nameTok.text;
            if (match(TokenType::L_PAREN)) {
                do {
                    Token keyTok = consume(TokenType::IDENTIFIER, "Expected annotation arg key");
                    consume(TokenType::EQUAL, "Expected '=' after annotation arg key");
                    Token valTok;
                    if (current.type == TokenType::STRING_LITERAL || current.type == TokenType::INTEGER_LITERAL || current.type == TokenType::FLOAT_LITERAL) {
                        valTok = current;
                        advance();
                    } else {
                        diag.error(SourceLocation::fromOffset(current.byteOffset), "Expected literal in annotation arg value");
                        throw ParseError();
                    }
                    annot.args[std::string(keyTok.text)] = std::string(valTok.text);
                } while (match(TokenType::COMMA));
                consume(TokenType::R_PAREN, "Expected ')' after annotation args");
            }
            annotations.push_back(std::move(annot));
        } while (match(TokenType::COMMA));
        consume(TokenType::R_BRACKET, "Expected ']' after annotations");
    }
    return annotations;
}

UseTreeNode Parser::parseUseTree() {
    UseTreeNode node;
    node.loc = SourceLocation::fromOffset(current.byteOffset);
    if (match(TokenType::STAR)) {
        node.isGlob = true;
        return node;
    }
    if (match(TokenType::L_BRACE)) {
        do {
            node.children.push_back(parseUseTree());
        } while (match(TokenType::COMMA));
        consume(TokenType::R_BRACE, "Expected '}' in use tree");
        return node;
    }
    if (current.type == TokenType::IDENTIFIER || current.type == TokenType::KW_PRINT || current.type == TokenType::STRING_LITERAL) {
        node.segments.push_back(current.text);
        advance();
    } else {
        diag.error(SourceLocation::fromOffset(current.byteOffset), "Expected identifier or string in use path");
        throw ParseError();
    }
    
    while (match(TokenType::COLON_COLON)) {
        if (match(TokenType::STAR)) {
            node.isGlob = true;
            break;
        }
        if (match(TokenType::L_BRACE)) {
            do {
                node.children.push_back(parseUseTree());
            } while (match(TokenType::COMMA));
            consume(TokenType::R_BRACE, "Expected '}' in use tree");
            break;
        }
        if (current.type == TokenType::IDENTIFIER || current.type == TokenType::KW_PRINT || current.type == TokenType::STRING_LITERAL) {
            node.segments.push_back(current.text);
            advance();
        } else {
            diag.error(SourceLocation::fromOffset(current.byteOffset), "Expected identifier or string in use path");
            throw ParseError();
        }
    }
    
    if (match(TokenType::KW_AS)) {
        Token aliasTok = consume(TokenType::IDENTIFIER, "Expected alias after 'as'");
        node.alias = aliasTok.text;
    }
    return node;
}

std::unique_ptr<DeclNode> Parser::parseUseDecl() {
    auto node = std::make_unique<UseDeclNode>();
    node->loc = SourceLocation::fromOffset(current.byteOffset);
    consume(TokenType::KW_USE, "Expected 'use'");
    node->tree = parseUseTree();
    consume(TokenType::SEMI, "Expected ';' after use declaration");
    return node;
}

std::unique_ptr<DeclNode> Parser::parseTypeAliasDecl() {
    auto node = std::make_unique<TypeAliasDeclNode>();
    node->loc = SourceLocation::fromOffset(current.byteOffset);
    consume(TokenType::KW_TYPE, "Expected 'type'");
    Token nameTok = consume(TokenType::IDENTIFIER, "Expected type alias name");
    node->name = nameTok.text;
    node->genericParams = parseGenericParams();
    consume(TokenType::EQUAL, "Expected '='");
    node->aliasedType = parseType();
    consume(TokenType::SEMI, "Expected ';' after type alias");
    return node;
}

std::unique_ptr<DeclNode> Parser::parseModDecl() {
    auto node = std::make_unique<ModDeclNode>();
    node->loc = SourceLocation::fromOffset(current.byteOffset);
    consume(TokenType::KW_MOD, "Expected 'mod'");
    Token nameTok = consume(TokenType::IDENTIFIER, "Expected module name");
    node->name = nameTok.text;
    consume(TokenType::L_BRACE, "Expected '{' for module body");
    while (!check(TokenType::R_BRACE) && !check(TokenType::END_OF_FILE)) {
        auto item = parseDeclaration();
        if (auto decl = dynamic_cast<DeclNode*>(item.get())) {
            item.release();
            node->decls.push_back(std::unique_ptr<DeclNode>(decl));
        } else {
            diag.error(SourceLocation::fromOffset(current.byteOffset), "Only declarations are allowed in module body");
            throw ParseError();
        }
    }
    consume(TokenType::R_BRACE, "Expected '}' for module body");
    return node;
}

std::unique_ptr<DeclNode> Parser::parseExternDecl() {
    auto node = std::make_unique<ExternDeclNode>();
    node->loc = SourceLocation::fromOffset(current.byteOffset);
    consume(TokenType::KW_EXTERN, "Expected 'extern'");
    auto func = parseFunctionDecl();
    auto fd = dynamic_cast<FunctionDeclNode*>(func.get());
    if (!fd) {
        diag.error(SourceLocation::fromOffset(current.byteOffset), "Expected function declaration after extern");
        throw ParseError();
    }
    func.release();
    node->func = std::unique_ptr<FunctionDeclNode>(fd);
    return node;
}

'''

content = content.replace('// DECLARATIONS\n// ==========================================', '// DECLARATIONS\n// ==========================================\n\n' + new_functions)

# 2. Update parseDeclaration
decl_replacement = r'''std::unique_ptr<ItemNode> Parser::parseDeclaration() {
    auto annots = parseAnnotations();
    
    bool isExported = false;
    if (match(TokenType::KW_EXPORT)) {
        isExported = true;
    }
    
    std::unique_ptr<DeclNode> decl = nullptr;
    
    bool isFunction = false;
    if (current.type == TokenType::KW_FN || current.type == TokenType::KW_ASYNC) {
        isFunction = true;
    } else if (current.type == TokenType::KW_COMPTIME) {
        if (peek.type == TokenType::KW_FN || peek.type == TokenType::KW_ASYNC) {
            isFunction = true;
        }
    }

    if (current.type == TokenType::KW_DEC || current.type == TokenType::KW_CONST) {
        decl = parseVarDecl();
    } else if (isFunction) {
        decl = parseFunctionDecl();
    } else if (current.type == TokenType::KW_STRUCT) {
        decl = parseStructDecl();
    } else if (current.type == TokenType::KW_ENUM) {
        decl = parseEnumDecl();
    } else if (current.type == TokenType::KW_TRAIT) {
        decl = parseTraitDecl();
    } else if (current.type == TokenType::KW_IMPL) {
        decl = parseImplDecl();
    } else if (current.type == TokenType::KW_USE) {
        decl = parseUseDecl();
    } else if (current.type == TokenType::KW_TYPE) {
        decl = parseTypeAliasDecl();
    } else if (current.type == TokenType::KW_MOD) {
        decl = parseModDecl();
    } else if (current.type == TokenType::KW_EXTERN) {
        decl = parseExternDecl();
    } else {
        if (!annots.empty() || isExported) {
            diag.error(SourceLocation::fromOffset(current.byteOffset), "Annotations or export not attached to a declaration");
            throw ParseError();
        }
        return parseStatement();
    }
    
    if (decl) {
        decl->isExported = isExported;
        decl->annotations = std::move(annots);
    }
    return decl;
}'''

content = re.sub(
    r'std::unique_ptr<ItemNode> Parser::parseDeclaration\(\) \{.*?return parseStatement\(\);\s*\}\s*\}',
    decl_replacement,
    content,
    flags=re.DOTALL
)

with open('src/FrontEnd/Parser.cpp', 'w') as f:
    f.write(content)

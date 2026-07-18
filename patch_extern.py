import re

with open('src/FrontEnd/Parser.cpp', 'r') as f:
    content = f.read()

# Update parseFunctionDecl signature and body
content = content.replace(
    'std::unique_ptr<DeclNode> Parser::parseFunctionDecl() {',
    'std::unique_ptr<DeclNode> Parser::parseFunctionDecl(bool allowEmptyBody) {'
)

content = content.replace(
    'if (match(TokenType::ARROW)) funcDecl->returnType = parseType();\n    funcDecl->body = parseBlockStatement();\n    return funcDecl;',
    'if (match(TokenType::ARROW)) funcDecl->returnType = parseType();\n    if (allowEmptyBody && match(TokenType::SEMI)) {\n        // Empty body allowed for extern functions\n    } else {\n        funcDecl->body = parseBlockStatement();\n    }\n    return funcDecl;'
)

# Update parseExternDecl
extern_replacement = r'''std::unique_ptr<DeclNode> Parser::parseExternDecl() {
    auto node = std::make_unique<ExternDeclNode>();
    node->loc = SourceLocation::fromOffset(current.byteOffset);
    consume(TokenType::KW_EXTERN, "Expected 'extern'");
    auto func = parseFunctionDecl(true); // true means allow empty body
    auto fd = dynamic_cast<FunctionDeclNode*>(func.get());
    if (!fd) {
        diag.error(SourceLocation::fromOffset(current.byteOffset), "Expected function declaration after extern");
        throw ParseError();
    }
    func.release();
    node->func = std::unique_ptr<FunctionDeclNode>(fd);
    return node;
}'''

content = re.sub(r'std::unique_ptr<DeclNode> Parser::parseExternDecl\(\) \{.*?return node;\s*\}', extern_replacement, content, flags=re.DOTALL)

with open('src/FrontEnd/Parser.cpp', 'w') as f:
    f.write(content)

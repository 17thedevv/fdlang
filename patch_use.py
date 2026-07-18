import re

with open('src/FrontEnd/Parser.cpp', 'r') as f:
    content = f.read()

replacement = r'''UseTreeNode Parser::parseUseTree() {
    UseTreeNode node;
    node.loc = SourceLocation::fromOffset(current.byteOffset);
    if (match(TokenType::MULTIPLY)) {
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
    
    // Allow optional ::
    if (match(TokenType::COLON_COLON)) {
        // do nothing, just consume
    }
    
    if (current.type == TokenType::MULTIPLY || current.type == TokenType::L_BRACE || current.type == TokenType::IDENTIFIER || current.type == TokenType::STRING_LITERAL) {
        if (match(TokenType::MULTIPLY)) {
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
        // It could be another segment, but wait, `as` is handled below.
    }
    
    if (match(TokenType::KW_AS)) {
        Token aliasTok = consume(TokenType::IDENTIFIER, "Expected alias after 'as'");
        node.alias = aliasTok.text;
    }
    return node;
}'''

content = re.sub(
    r'UseTreeNode Parser::parseUseTree\(\) \{.*?return node;\s*\}',
    replacement,
    content,
    flags=re.DOTALL
)

with open('src/FrontEnd/Parser.cpp', 'w') as f:
    f.write(content)

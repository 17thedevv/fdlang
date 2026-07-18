import re

with open('src/FrontEnd/Parser.cpp', 'r') as f:
    content = f.read()

# Fix MULTIPLY_ASSIGN
content = content.replace('TokenType::MULTIPLY_ASSIGN', 'TokenType::STAR_ASSIGN')

# Fix Annotation parsing
annot_replacement = r'''std::vector<AnnotationNode> Parser::parseAnnotations() {
    std::vector<AnnotationNode> annotations;
    while (match(TokenType::AT_BRACKET)) {
        do {
            AnnotationNode annot;
            // No loc on AnnotationNode
            Token nameTok = consume(TokenType::IDENTIFIER, "Expected annotation name");
            annot.name = nameTok.text;
            if (match(TokenType::L_PAREN)) {
                do {
                    AnnotationArg arg;
                    if (current.type == TokenType::IDENTIFIER && peek.type == TokenType::EQUAL) {
                        Token keyTok = consume(TokenType::IDENTIFIER, "Expected annotation arg key");
                        consume(TokenType::EQUAL, "Expected '=' after annotation arg key");
                        arg.key = std::string(keyTok.text);
                    }
                    arg.value = parseExpression(false);
                    annot.args.push_back(std::move(arg));
                } while (match(TokenType::COMMA));
                consume(TokenType::R_PAREN, "Expected ')' after annotation args");
            }
            annotations.push_back(std::move(annot));
        } while (match(TokenType::COMMA));
        consume(TokenType::R_BRACKET, "Expected ']' after annotations");
    }
    return annotations;
}'''

content = re.sub(
    r'std::vector<AnnotationNode> Parser::parseAnnotations\(\) \{.*?return annotations;\s*\}',
    annot_replacement,
    content,
    flags=re.DOTALL
)

with open('src/FrontEnd/Parser.cpp', 'w') as f:
    f.write(content)

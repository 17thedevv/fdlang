import re

with open('src/FrontEnd/Parser.cpp', 'r') as f:
    content = f.read()

content = content.replace(
    'if (current.type == TokenType::MULTIPLY || current.type == TokenType::L_BRACE || current.type == TokenType::IDENTIFIER || current.type == TokenType::STRING_LITERAL) {',
    'std::cout << "[DEBUG] parseUseTree after segment: " << (int)current.type << " " << std::string(current.text) << "\\n";\n    if (current.type == TokenType::MULTIPLY || current.type == TokenType::L_BRACE || current.type == TokenType::IDENTIFIER || current.type == TokenType::STRING_LITERAL) {'
)

with open('src/FrontEnd/Parser.cpp', 'w') as f:
    f.write(content)

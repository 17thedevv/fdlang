import re

with open('src/FrontEnd/Parser.cpp', 'r') as f:
    content = f.read()

content = content.replace(
    'consume(TokenType::SEMI, "Expected \';\' after use declaration");',
    'if (current.type != TokenType::SEMI) { std::cout << "[DEBUG] parseUseDecl expected SEMI but got: " << (int)current.type << " text: " << std::string(current.text) << "\\n"; } consume(TokenType::SEMI, "Expected \';\' after use declaration");'
)

with open('src/FrontEnd/Parser.cpp', 'w') as f:
    f.write(content)

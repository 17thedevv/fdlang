import re

with open('src/FrontEnd/Parser.cpp', 'r') as f:
    content = f.read()

replacement = r'''    if (current.type != TokenType::SEMI) {
        std::cout << "[DEBUG] Expected SEMI. prev text: " << std::string(prev.text) << " current text: " << std::string(current.text) << "\n";
    }
    consume(TokenType::SEMI, "Expected ';' after expression");'''

content = re.sub(
    r'consume\(TokenType::SEMI, "Expected \';\' after expression"\);',
    replacement,
    content
)

with open('src/FrontEnd/Parser.cpp', 'w') as f:
    f.write(content)

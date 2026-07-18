import re

with open('src/FrontEnd/Parser.cpp', 'r') as f:
    content = f.read()

# Replace AT with AT_BRACKET
content = content.replace('match(TokenType::AT)', 'match(TokenType::AT_BRACKET)')
content = content.replace('consume(TokenType::L_BRACKET, "Expected \'[\' after \'@\'");', '') # since AT_BRACKET is `@[`

# Replace STAR with MULTIPLY
content = content.replace('TokenType::STAR', 'TokenType::MULTIPLY')

with open('src/FrontEnd/Parser.cpp', 'w') as f:
    f.write(content)

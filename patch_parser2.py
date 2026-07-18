import re

with open('src/FrontEnd/Parser.cpp', 'r') as f:
    content = f.read()

# 1. parseValuePath
content = re.sub(
    r'if \(current\.type != TokenType::IDENTIFIER\) \{.*?Token first = consume\(TokenType::IDENTIFIER, "Expected identifier in path"\);\s*ident->segments\.push_back\(first\.text\);',
    r'Token first;\n            if (current.type == TokenType::IDENTIFIER || current.type == TokenType::KW_PRINT) {\n                first = current;\n                advance();\n            } else {\n                diag.error(SourceLocation{current.byteOffset, 0, 0}, "Expected identifier in path");\n                throw ParseError();\n            }\n            ident->segments.push_back(first.text);',
    content,
    flags=re.DOTALL
)

# 2. parsePostfix
content = re.sub(
    r'// Member Access or Method Call\s*Token memberTok = consume\(TokenType::IDENTIFIER, "Expected member name"\);',
    r'// Member Access or Method Call\n            Token memberTok;\n            if (current.type == TokenType::IDENTIFIER || current.type == TokenType::KW_PRINT) {\n                memberTok = current;\n                advance();\n            } else {\n                diag.error(SourceLocation{current.byteOffset, 0, 0}, "Expected member name");\n                throw ParseError();\n            }',
    content,
    flags=re.DOTALL
)

# 3. parseNamedType
content = re.sub(
    r'\} else \{\s*idTok = consume\(TokenType::IDENTIFIER, "Expected identifier or Self in type path"\);\s*\}',
    r'} else if (current.type == TokenType::IDENTIFIER || current.type == TokenType::KW_PRINT) {\n              idTok = current;\n              advance();\n          } else {\n              diag.error(SourceLocation{current.byteOffset, 0, 0}, "Expected identifier or Self in type path");\n              throw ParseError();\n          }',
    content,
    flags=re.DOTALL
)

# 4. parseUseTree (two places)
content = re.sub(
    r'node\.segments\.push_back\(consume\(TokenType::IDENTIFIER, "Expected identifier in path"\)\.text\);',
    r'if (current.type == TokenType::IDENTIFIER || current.type == TokenType::KW_PRINT) { node.segments.push_back(current.text); advance(); } else { diag.error(SourceLocation{current.byteOffset, 0, 0}, "Expected identifier in path"); throw ParseError(); }',
    content
)
content = re.sub(
    r'node\.segments\.push_back\(consume\(TokenType::IDENTIFIER, ""\)\.text\);',
    r'if (current.type == TokenType::IDENTIFIER || current.type == TokenType::KW_PRINT) { node.segments.push_back(current.text); advance(); } else { diag.error(SourceLocation{current.byteOffset, 0, 0}, "Expected identifier in use tree"); throw ParseError(); }',
    content
)

with open('src/FrontEnd/Parser.cpp', 'w') as f:
    f.write(content)

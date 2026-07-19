import re

with open('src/MiddleEnd/TypeChecker.cpp', 'r') as f:
    content = f.read()

# Replace actual newlines inside strings with \\n
content = re.sub(
    r'printf\("([^"]*)\n([^"]*)"\)',
    r'printf("\1\\n\2")',
    content
)
content = re.sub(
    r'printf\("([^"]*)\n([^"]*)"\)',
    r'printf("\1\\n\2")',
    content
)

with open('src/MiddleEnd/TypeChecker.cpp', 'w') as f:
    f.write(content)
print("Fixed newlines in string literals")

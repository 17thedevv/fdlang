import re

with open('src/MiddleEnd/TypeChecker.cpp', 'r') as f:
    content = f.read()

content = re.sub(
    r"bool TypeChecker::check\(ASTNode\* root\) \{",
    r'bool TypeChecker::check(ASTNode* root) {\n    printf("[DEBUG] Entered TypeChecker::check\\n"); fflush(stdout);',
    content
)

with open('src/MiddleEnd/TypeChecker.cpp', 'w') as f:
    f.write(content)
print("Added Entered TypeChecker::check")

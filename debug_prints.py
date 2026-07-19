import re

with open('src/MiddleEnd/MonomorphizationEngine.cpp', 'r') as f:
    content = f.read()

content = re.sub(
    r"(SymbolID MonomorphizationEngine::requestStructSpecialization\([^\)]+\)\s*\{)",
    r"\1\n    printf(\"[DEBUG] requestStructSpecialization: %s\\n\", genericTemplate->name.c_str()); fflush(stdout);",
    content
)

with open('src/MiddleEnd/MonomorphizationEngine.cpp', 'w') as f:
    f.write(content)

with open('src/MiddleEnd/TypeChecker.cpp', 'r') as f:
    content = f.read()

content = re.sub(
    r"(bool check\(ASTNode\* root\)\s*\{)",
    r"\1\n        static int check_depth = 0; check_depth++; printf(\"[DEBUG] check_depth=%d\\n\", check_depth); fflush(stdout);",
    content
)

with open('src/MiddleEnd/TypeChecker.cpp', 'w') as f:
    f.write(content)

print("Added debug prints")

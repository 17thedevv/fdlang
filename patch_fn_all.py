import re

with open('src/MiddleEnd/TypeChecker.cpp', 'r') as f:
    content = f.read()

# TypePrePass FunctionDeclNode
content = re.sub(
    r"(class TypePrePass.*?)(void visit\(FunctionDeclNode& node\) override \{\s*)(std::vector<std::string> paramNames;)",
    r"\1\2if (!node.genericParams.empty()) return;\n              \3",
    content,
    flags=re.DOTALL
)

# ConstraintGenerator FunctionDeclNode
content = re.sub(
    r"(class ConstraintGenerator.*?)(void visit\(FunctionDeclNode& node\) override \{\s*)(const Type\* oldRet = currentReturnType;)",
    r"\1\2if (!node.genericParams.empty()) return;\n              \3",
    content,
    flags=re.DOTALL
)

with open('src/MiddleEnd/TypeChecker.cpp', 'w') as f:
    f.write(content)
print("Patched FunctionDeclNode skips")

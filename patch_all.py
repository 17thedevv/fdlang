import re

with open('src/MiddleEnd/TypeChecker.cpp', 'r') as f:
    content = f.read()

# TypePrePass ImplDeclNode
content = re.sub(
    r"(void visit\(ImplDeclNode& node\) override \{\s*)(const Type\* selfType = evaluateTypeNode\(node.selfType.get\(\)\);)",
    r"\1if (!node.genericParams.empty()) return;\n              \2",
    content
)

# ConstraintGenerator ImplDeclNode
content = re.sub(
    r"(class ConstraintGenerator.*?)(void visit\(ImplDeclNode& node\) override \{\s*)(for \(auto& method : node.methods\) method->accept\(\*this\);\s*\})",
    r"\1\2if (!node.genericParams.empty()) return;\n              \3",
    content,
    flags=re.DOTALL
)

# TypeResolver ImplDeclNode
content = re.sub(
    r"(class TypeResolver.*?)(void visit\(ImplDeclNode& node\) override \{\s*)(for \(auto& method : node.methods\) method->accept\(\*this\);\s*\})",
    r"\1\2if (!node.genericParams.empty()) return;\n              \3",
    content,
    flags=re.DOTALL
)

with open('src/MiddleEnd/TypeChecker.cpp', 'w') as f:
    f.write(content)
print("Patched ImplDeclNode skips")

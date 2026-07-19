import re

with open('src/MiddleEnd/TypeChecker.cpp', 'r') as f:
    content = f.read()

content = re.sub(
    r"void visit\(ImplDeclNode& node\) override \{\s*if \(!node\.genericParams\.empty\(\)\) \{",
    r'void visit(ImplDeclNode& node) override {\n            printf("[DEBUG] TypePrePass::visit(ImplDeclNode)\\n");\n            if (!node.genericParams.empty()) {',
    content
)

with open('src/MiddleEnd/TypeChecker.cpp', 'w') as f:
    f.write(content)
print("Added printf")

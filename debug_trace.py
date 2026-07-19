import re

with open('src/MiddleEnd/TypeChecker.cpp', 'r') as f:
    content = f.read()

content = re.sub(
    r"void visit\(StructDeclNode& node\) override \{",
    r'void visit(StructDeclNode& node) override {\n            printf("[DEBUG] TypePrePass::visit(StructDeclNode) %s\\n", node.name.c_str());',
    content
)

content = re.sub(
    r"void visit\(FunctionDeclNode& node\) override \{",
    r'void visit(FunctionDeclNode& node) override {\n            printf("[DEBUG] TypePrePass::visit(FunctionDeclNode) %s\\n", node.name.c_str());',
    content
)

content = re.sub(
    r"void visit\(NamedTypeNode& node\) override \{",
    r'void visit(NamedTypeNode& node) override {\n            printf("[DEBUG] TypePrePass::visit(NamedTypeNode)\\n");',
    content
)

with open('src/MiddleEnd/TypeChecker.cpp', 'w') as f:
    f.write(content)
print("Added trace prints")

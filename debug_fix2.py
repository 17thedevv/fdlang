import re

with open('src/MiddleEnd/TypeChecker.cpp', 'r') as f:
    content = f.read()

content = re.sub(
    r'printf\("\[DEBUG\] TypePrePass::visit\(StructDeclNode\) %s\\n", std::string\(node\.name\)\.c_str\(\)\);',
    r'printf("[DEBUG] TypePrePass::visit(StructDeclNode)\\n"); fflush(stdout);',
    content
)

content = re.sub(
    r'printf\("\[DEBUG\] TypePrePass::visit\(FunctionDeclNode\) %s\\n", std::string\(node\.name\)\.c_str\(\)\);',
    r'printf("[DEBUG] TypePrePass::visit(FunctionDeclNode)\\n"); fflush(stdout);',
    content
)

content = re.sub(
    r'printf\("\[DEBUG\] TypePrePass::visit\(NamedTypeNode\)\\n"\);',
    r'printf("[DEBUG] TypePrePass::visit(NamedTypeNode)\\n"); fflush(stdout);',
    content
)

with open('src/MiddleEnd/TypeChecker.cpp', 'w') as f:
    f.write(content)
print("Removed string conversion in debug prints")

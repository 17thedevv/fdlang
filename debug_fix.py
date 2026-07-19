import re

with open('src/MiddleEnd/TypeChecker.cpp', 'r') as f:
    content = f.read()

content = re.sub(
    r"printf\(\"\[DEBUG\] TypePrePass::visit\(StructDeclNode\) %s\\n\", node\.name\.c_str\(\)\);",
    r'printf("[DEBUG] TypePrePass::visit(StructDeclNode) %s\\n", std::string(node.name).c_str());',
    content
)

content = re.sub(
    r"printf\(\"\[DEBUG\] TypePrePass::visit\(FunctionDeclNode\) %s\\n\", node\.name\.c_str\(\)\);",
    r'printf("[DEBUG] TypePrePass::visit(FunctionDeclNode) %s\\n", std::string(node.name).c_str());',
    content
)

with open('src/MiddleEnd/TypeChecker.cpp', 'w') as f:
    f.write(content)
print("Fixed trace prints")

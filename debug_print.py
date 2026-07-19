import re

with open('src/MiddleEnd/TypeChecker.cpp', 'r') as f:
    content = f.read()

content = re.sub(
    r"(void visit\(ImplDeclNode& node\) override \{\s*)(if \(!node\.genericParams\.empty\(\)\) return;\s*)",
    r'\1printf("[DEBUG] TypePrePass::visit(ImplDeclNode) genericParams size=%zu\\n", node.genericParams.size());\n              \2',
    content,
    count=1
)

with open('src/MiddleEnd/TypeChecker.cpp', 'w') as f:
    f.write(content)
print("Added debug print")

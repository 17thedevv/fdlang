import re
with open('src/MiddleEnd/TypeChecker.cpp', 'r') as f:
    content = f.read()

content = re.sub(
    r"(\s*)void visit\(NamedTypeNode& node\) override \{",
    r"\1void visit(NamedTypeNode& node) override {\n                printf(\"[DEBUG] ConstraintGenerator::visit(NamedTypeNode) symbolId=%d\\n\", node.symbolId); fflush(stdout);",
    content
)

with open('src/MiddleEnd/TypeChecker.cpp', 'w') as f:
    f.write(content)
print("Patched TypeChecker with printf")

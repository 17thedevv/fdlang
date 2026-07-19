import re

with open('src/MiddleEnd/SubstitutionVisitor.cpp', 'r') as f:
    content = f.read()

# Add visit(ImplDeclNode&)
pat = r"(void SubstitutionVisitor::visit\(StructFieldNode& n\) \{\s*n\.type = substituteType\(std::move\(n\.type\)\);\s*\})"
repl = r"\1\n\nvoid SubstitutionVisitor::visit(ImplDeclNode& n) {\n    n.targetType = substituteType(std::move(n.targetType));\n    n.traitType = substituteType(std::move(n.traitType));\n    for (auto& m : n.methods) m->accept(*this);\n}"

content = re.sub(pat, repl, content)

with open('src/MiddleEnd/SubstitutionVisitor.cpp', 'w') as f:
    f.write(content)
print("Patched SubstitutionVisitor.cpp")

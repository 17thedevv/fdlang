import re

with open('include/mellis/MiddleEnd/SubstitutionVisitor.h', 'r') as f:
    content = f.read()

content = content.replace("void visit(ImplDeclNode&) override {}", "void visit(ImplDeclNode&) override;")

with open('include/mellis/MiddleEnd/SubstitutionVisitor.h', 'w') as f:
    f.write(content)
print("Patched SubstitutionVisitor.h")

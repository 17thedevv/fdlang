
import re

with open("src/MiddleEnd/TypeChecker.cpp", "r") as f:
    content = f.read()

# First, remove the unwanted visit(IdentifierExpr) from TypePrePass
# Find TypePrePass::visit(LiteralExpr) and its vicinity
typeprepass_start = content.find("class TypePrePass")
if typeprepass_start != -1:
    ident_start = content.find("void visit(IdentifierExpr& node) override {", typeprepass_start)
    if ident_start != -1 and ident_start < content.find("class ConstraintGenerator"):
        ident_end = content.find("void visit(BinaryExpr& node)", ident_start)
        # replace the whole block with the original empty visit
        content = content[:ident_start] + "void visit(IdentifierExpr& node) override { resolve(node.inferredType, node.loc); }\n        " + content[ident_end:]

# Now, add visit(IdentifierExpr) to ConstraintGenerator properly
cg_start = content.find("class ConstraintGenerator")
if cg_start != -1:
    cg_ident_start = content.find("void visit(IdentifierExpr& node)", cg_start)
    if cg_ident_start != -1:
        cg_ident_end = content.find("void visit(BinaryExpr& node)", cg_ident_start)
        new_cg_ident = """void visit(IdentifierExpr& node) override {
            if (node.resolvedSymbol != kInvalidSymbolID) {
                const Type* ty = typeTable[node.resolvedSymbol];
                if (ty) node.inferredType = ty;
            }
        }
        """
        content = content[:cg_ident_start] + new_cg_ident + content[cg_ident_end:]

with open("src/MiddleEnd/TypeChecker.cpp", "w") as f:
    f.write(content)

print("Fixed TypeChecker.cpp!")

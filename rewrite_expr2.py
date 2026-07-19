
import re

with open("src/MiddleEnd/TypeChecker.cpp", "r") as f:
    content = f.read()

# Add visit(IdentifierExpr) to ConstraintGenerator
# Find where visit(VarDeclNode) is to add it near there.
start_idx = content.find("void visit(FunctionDeclNode& node) override {")

new_method = """void visit(IdentifierExpr& node) override {
            if (node.resolvedSymbol != kInvalidSymbolID) {
                const Type* ty = typeTable[node.resolvedSymbol];
                if (ty) node.inferredType = ty;
            }
        }
        
        """
content = content[:start_idx] + new_method + content[start_idx:]

with open("src/MiddleEnd/TypeChecker.cpp", "w") as f:
    f.write(content)

print("Rewritten ConstraintGenerator::visit(IdentifierExpr) successfully!")


import re

with open("src/MiddleEnd/TypeChecker.cpp", "r") as f:
    content = f.read()

start_idx = content.find("void visit(IdentifierExpr& node) override {")
if start_idx != -1:
    end_idx = content.find("void visit(BinaryExpr& node) override {", start_idx)
    new_method = """void visit(IdentifierExpr& node) override {
            if (node.resolvedSymbol != kInvalidSymbolID) {
                const Type* ty = typeTable[node.resolvedSymbol];
                if (ty) {
                    std::cerr << "[DEBUG] ConstraintGenerator::visit(IdentifierExpr) " << node.segments[0] << " gets type " << ty->toString() << "\\n";
                    node.inferredType = ty;
                } else {
                    std::cerr << "[DEBUG] ConstraintGenerator::visit(IdentifierExpr) " << node.segments[0] << " typeTable is NULL!\\n";
                }
            } else {
                std::cerr << "[DEBUG] ConstraintGenerator::visit(IdentifierExpr) " << node.segments[0] << " resolvedSymbol is kInvalidSymbolID!\\n";
            }
        }
        """
    content = content[:start_idx] + new_method + content[end_idx:]
    with open("src/MiddleEnd/TypeChecker.cpp", "w") as f:
        f.write(content)
    print("Added printf!")
else:
    print("Could not find visit(IdentifierExpr)!")

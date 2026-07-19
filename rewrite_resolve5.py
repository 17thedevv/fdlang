
import re

with open("src/MiddleEnd/Resolver.cpp", "r") as f:
    content = f.read()

old_code = """    void visit(IdentifierExpr& node) override { 
        // Resolve the first segment as a symbol in the current scope
        if (!node.segments.empty()) {
            SymbolID id = sm.resolve(node.segments[0], node.loc);"""
            
new_code = """    void visit(IdentifierExpr& node) override { 
        if (!node.segments.empty()) {
            SymbolID id = sm.resolvePath(node.segments, node.loc);"""

content = content.replace(old_code, new_code)

with open("src/MiddleEnd/Resolver.cpp", "w") as f:
    f.write(content)

print("Rewritten Resolver IdentifierExpr!")

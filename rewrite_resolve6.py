
import re

with open("src/MiddleEnd/Resolver.cpp", "r") as f:
    content = f.read()

old_code1 = """        for (auto& param : node.genericParams) {}"""
new_code1 = """        for (auto& param : node.genericParams) {
            param.symbolId = sm.declare(param.name, SymbolKind::GenericParam, param.loc, nullptr);
        }"""
        
content = content.replace(old_code1, new_code1)

old_code2 = """    void visit(EnumDeclNode& node) override { 
        sm.enterExistingScope(node.bodyScopeId);
        for (auto& v : node.variants) {"""
        
new_code2 = """    void visit(EnumDeclNode& node) override { 
        sm.enterExistingScope(node.bodyScopeId);
        for (auto& gp : node.genericParams) {
            for (auto& bound : gp.bounds) {
                bound->accept(static_cast<TypeVisitor&>(*this));
            }
        }
        for (auto& v : node.variants) {"""
        
content = content.replace(old_code2, new_code2)

with open("src/MiddleEnd/Resolver.cpp", "w") as f:
    f.write(content)

print("Rewritten Resolver EnumDeclNode generics!")

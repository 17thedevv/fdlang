import sys
import re

with open('src/MiddleEnd/Resolver.cpp', 'r', encoding='utf-8') as f:
    lines = f.read().splitlines()

out = []
in_res_for = False

for l in lines:
    if 'void visit(ParamDeclNode& node) override {' in l and 'Handled in FunctionDeclNode' in l:
        out.append('    void visit(ParamDeclNode& node) override {')
        out.append('        if (node.symbolId == kInvalidSymbolID) {')
        out.append('            node.symbolId = sm.declare(node.name, SymbolKind::Variable, node.loc, &node);')
        out.append('        }')
        out.append('        if (node.type) node.type->accept(static_cast<TypeVisitor&>(*this));')
        out.append('    }')
        continue
    
    if 'for (auto& p : node.params) {' in l:
        # Check if we are inside FunctionDeclNode
        # In FunctionDeclNode it does: if (p->type) p->type->accept(...)
        pass
    
    if 'if (p->type) p->type->accept(static_cast<TypeVisitor&>(*this));' in l:
        out.append('            p->accept(static_cast<ASTVisitor&>(*this));')
        continue

    if 'if (node.iterable) node.iterable->accept(static_cast<ASTVisitor&>(*this));' in l:
        out.append('            if (node.iterable) node.iterable->accept(static_cast<ASTVisitor&>(*this));')
        out.append('            if (node.bindingId == kInvalidSymbolID) {')
        out.append('                node.bindingId = sm.declare(node.bindingName, SymbolKind::Variable, node.loc, &node);')
        out.append('            }')
        continue
        
    out.append(l)

with open('src/MiddleEnd/Resolver.cpp', 'w', encoding='utf-8') as f:
    f.write('\n'.join(out))

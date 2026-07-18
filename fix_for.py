import sys
with open('src/MiddleEnd/Resolver.cpp', 'r', encoding='utf-8') as f:
    lines = f.read().splitlines()

out = []
in_decl_for = False
in_res_for = False

for i, l in enumerate(lines):
    if 'void visit(ForStmtNode& node) override {' in l:
        if i < 200:
            in_decl_for = True
            out.append('    void visit(ForStmtNode& node) override { std::cout << "[Resolver] " << __FUNCTION__ << " line " << __LINE__ << "\\n"; ')
            out.append('        node.bodyScopeId = sm.table.createScope(ScopeKind::Loop, sm.scopeStack.current());')
            out.append('        sm.enterExistingScope(node.bodyScopeId);')
            out.append('        if (node.kind == ForKind::CStyle) {')
            out.append('            if (node.init) node.init->accept(*this);')
            out.append('        } else {')
            out.append('            node.bindingId = sm.declare(node.bindingName, SymbolKind::Variable, node.loc, &node);')
            out.append('        }')
            out.append('        if (node.body) node.body->accept(*this);')
            out.append('        sm.exitScope();')
            out.append('    }')
        else:
            in_res_for = True
            out.append('    void visit(ForStmtNode& node) override { std::cout << "[Resolver] " << __FUNCTION__ << " line " << __LINE__ << "\\n"; ')
            out.append('        if (node.bodyScopeId == kInvalidSymbolID) {')
            out.append('            node.bodyScopeId = sm.table.createScope(ScopeKind::Loop, sm.scopeStack.current());')
            out.append('        }')
            out.append('        if (node.kind == ForKind::CStyle) {')
            out.append('            sm.enterExistingScope(node.bodyScopeId);')
            out.append('            if (node.init) node.init->accept(static_cast<ASTVisitor&>(*this));')
            out.append('            if (node.cond) node.cond->accept(static_cast<ASTVisitor&>(*this));')
            out.append('            if (node.step) node.step->accept(static_cast<ASTVisitor&>(*this));')
            out.append('            if (node.body) node.body->accept(static_cast<ASTVisitor&>(*this));')
            out.append('            sm.exitScope();')
            out.append('        } else {')
            out.append('            if (node.iterable) node.iterable->accept(static_cast<ASTVisitor&>(*this));')
            out.append('            sm.enterExistingScope(node.bodyScopeId);')
            out.append('            if (node.body) node.body->accept(static_cast<ASTVisitor&>(*this));')
            out.append('            sm.exitScope();')
            out.append('        }')
            out.append('    }')
        continue

    if in_decl_for:
        if l.strip() == '}':
            in_decl_for = False
        continue
    if in_res_for:
        if l.strip() == '}':
            in_res_for = False
        continue
    out.append(l)

with open('src/MiddleEnd/Resolver.cpp', 'w', encoding='utf-8') as f:
    f.write('\n'.join(out))

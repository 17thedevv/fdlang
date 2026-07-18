import sys

content = open('src/MiddleEnd/Resolver.cpp', 'r', encoding='utf-8').read()

decl_visitor_stmts = """
    void visit(UnsafeStmtNode& node) override {
        if (node.body) node.body->accept(*this);
    }
    void visit(ComptimeStmtNode& node) override {
        if (node.body) node.body->accept(*this);
    }
"""

decl_visitor_exprs = """
    void visit(ArrayLiteralExpr&) override {}
    void visit(TupleLiteralExpr&) override {}
    void visit(SizeofExpr&) override {}
    void visit(AlignofExpr&) override {}
"""

res_visitor_stmts = """
    void visit(UnsafeStmtNode& node) override {
        if (node.body) node.body->accept(*this);
    }
    void visit(ComptimeStmtNode& node) override {
        if (node.body) node.body->accept(*this);
    }
"""

res_visitor_exprs = """
    void visit(ArrayLiteralExpr& node) override {
        for (auto& el : node.elements) el->accept(*this);
    }
    void visit(TupleLiteralExpr& node) override {
        for (auto& el : node.elements) el->accept(*this);
    }
    void visit(SizeofExpr& node) override {
        if (node.targetType) node.targetType->accept(*this);
    }
    void visit(AlignofExpr& node) override {
        if (node.targetType) node.targetType->accept(*this);
    }
"""

type_visitor_types = """
    void visit(NeverTypeNode&) override {}
    void visit(TraitObjectTypeNode& node) override {
        if (node.trait) node.trait->accept(*this);
    }
"""

# Patching DeclarationVisitor statements
content = content.replace(
    'void visit(ContinueStmtNode& node) override {}',
    'void visit(ContinueStmtNode& node) override {}\n' + decl_visitor_stmts
)

# Patching DeclarationVisitor exprs
content = content.replace(
    'void visit(AwaitExpr&) override {}',
    'void visit(AwaitExpr&) override {}\n' + decl_visitor_exprs
)

# Patching ResolutionVisitor statements
content = content.replace(
    'void visit(ContinueStmtNode& node) override {',
    'void visit(ContinueStmtNode& node) override {\n    }\n' + res_visitor_stmts,
    1 # Be careful, ResolutionVisitor ContinueStmtNode has a body
)
# Wait, let's look for a better anchor in ResolutionVisitor statements

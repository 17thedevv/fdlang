import sys
content = open('src/MiddleEnd/TypeChecker.cpp', 'r', encoding='utf-8').read()

idx1 = content.find('    class TypeResolver : public ASTVisitor {')
idx2 = content.find('    MethodResolver methodResolver;', idx1)

repl = """    class TypeResolver : public ASTVisitor {
        SymbolTable& table;
        DiagnosticEngine& diag;
        TypeContext& ctx;
    public:
        TypeResolver(SymbolTable& table, DiagnosticEngine& diag, TypeContext& ctx) : table(table), diag(diag), ctx(ctx) {}

        void resolve(const Type*& t, SourceLocation loc) {
            if (!t) return;
            t = ctx.unificationTable.deepResolve(t, ctx);
            if (t->getKind() == TypeKind::InferenceVar) {
                diag.error(loc, "Type annotation needed");
            }
        }

        void visit(ProgramNode& node) override { for (auto& item : node.items) item->accept(*this); }
        void visit(ModDeclNode& node) override { for (auto& d : node.decls) d->accept(*this); }
        void visit(ExternDeclNode& node) override { if (node.func) node.func->accept(*this); }
        void visit(VarDeclNode& node) override {
            if (node.initializer) node.initializer->accept(*this);
        }
        void visit(ParamDeclNode& node) override {}
        void visit(FunctionDeclNode& node) override { if (node.body) node.body->accept(*this); }
        void visit(StructDeclNode& node) override {}
        void visit(StructFieldNode& node) override {}
        void visit(EnumDeclNode& node) override {}
        void visit(EnumVariantNode& node) override {}
        void visit(TraitDeclNode& node) override {}
        void visit(ImplDeclNode& node) override {}
        void visit(TypeAliasDeclNode& node) override {}
        void visit(UseDeclNode& node) override {}

        void visit(BlockStmtNode& node) override { for (auto& s : node.body) s->accept(*this); }
        void visit(ExprStmtNode& node) override { node.expr->accept(*this); }
        void visit(IfStmtNode& node) override {
            node.condition->accept(*this);
            node.thenBranch->accept(*this);
            if (node.elseBranch) node.elseBranch->accept(*this);
        }
        void visit(WhileStmtNode& node) override {
            node.condition->accept(*this);
            node.body->accept(*this);
        }
        void visit(ForStmtNode& node) override {
            if (node.init) node.init->accept(*this);
            if (node.cond) node.cond->accept(*this);
            if (node.step) node.step->accept(*this);
            if (node.iterable) node.iterable->accept(*this);
            if (node.body) node.body->accept(*this);
        }
        void visit(ReturnStmtNode& node) override { if (node.value) node.value->accept(*this); }
        void visit(BreakStmtNode& node) override {}
        void visit(ContinueStmtNode& node) override {}
        void visit(DeferStmtNode& node) override { node.call->accept(*this); }
        void visit(UnsafeStmtNode& node) override {}
        void visit(ComptimeStmtNode& node) override {}

        void visit(LiteralExpr& node) override { resolve(node.inferredType, node.loc); }
        void visit(IdentifierExpr& node) override { resolve(node.inferredType, node.loc); }
        void visit(BinaryExpr& node) override {
            node.left->accept(*this);
            node.right->accept(*this);
            resolve(node.inferredType, node.loc);
        }
        void visit(UnaryExpr& node) override {
            node.operand->accept(*this);
            resolve(node.inferredType, node.loc);
        }
        void visit(AssignExpr& node) override {
            node.lvalue->accept(*this);
            node.value->accept(*this);
            resolve(node.inferredType, node.loc);
        }
        void visit(CallExpr& node) override {
            node.callee->accept(*this);
            for (auto& arg : node.args) {
                if (arg.value) arg.value->accept(*this);
            }
            resolve(node.inferredType, node.loc);
        }
        void visit(MethodCallExpr& node) override {
            node.object->accept(*this);
            for (auto& arg : node.args) {
                if (arg.value) arg.value->accept(*this);
            }
            resolve(node.inferredType, node.loc);
        }
        void visit(IndexExpr& node) override {
            node.base->accept(*this);
            node.index->accept(*this);
            resolve(node.inferredType, node.loc);
        }
        void visit(MemberExpr& node) override {
            node.object->accept(*this);
            resolve(node.inferredType, node.loc);
            
            const Type* objTy = node.object->inferredType;
            if (auto* refTy = dynamic_cast<const ReferenceType*>(objTy)) objTy = refTy->pointee;
            else if (auto* ptrTy = dynamic_cast<const PointerType*>(objTy)) objTy = ptrTy->pointee;
            
            if (auto* st = dynamic_cast<const StructType*>(objTy)) {
                auto sym = table.getSymbol(st->structSymbolId);
                if (sym.kind == SymbolKind::Struct && sym.decl) {
                    auto* structDecl = static_cast<StructDeclNode*>(sym.decl);
                    for (size_t i = 0; i < structDecl->fields.size(); ++i) {
                        if (structDecl->fields[i]->name == node.member) {
                            node.resolvedFieldIndex = i;
                            break;
                        }
                    }
                }
            }
        }
        void visit(CastExpr& node) override {}
        void visit(ArrayLiteralExpr& node) override {}
        void visit(TupleLiteralExpr& node) override {}
        void visit(StructInitExpr& node) override {}
        void visit(MatchExpr& node) override {
            node.subject->accept(*this);
            for (auto& arm : node.arms) {
                if (arm.body) arm.body->accept(*this);
            }
            resolve(node.inferredType, node.loc);
        }
        void visit(LambdaExpr& node) override {}
        void visit(AwaitExpr& node) override {}
        void visit(SizeofExpr& node) override {}
        void visit(AlignofExpr& node) override {}
    };
"""
content = content[:idx1] + repl + content[idx2:]
content = content.replace('    TypeResolver resolver(diag_, ctx_);', '    TypeResolver resolver(table_, diag_, ctx_);')

idx_eof = content.find('} // namespace fl')
content = content[:idx_eof] + '''
const Type* TypeChecker::typeOf(SymbolID sym) const {
    if (sym < typeTable_.size()) return typeTable_[sym];
    return nullptr;
}

} // namespace fl
'''
with open('src/MiddleEnd/TypeChecker.cpp', 'w', encoding='utf-8') as f:
    f.write(content)

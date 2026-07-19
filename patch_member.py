import re

with open('src/MiddleEnd/TypeChecker.cpp', 'r') as f:
    content = f.read()

replacement = """        void visit(MemberExpr& node) override {
            node.base->accept(*this);
            const Type* baseTy = node.base->inferredType;
            if (baseTy) baseTy = ctx.unificationTable.deepResolve(baseTy, ctx);

            bool isDeref = false;
            while (auto* refTy = dynamic_cast<const ReferenceType*>(baseTy)) {
                baseTy = refTy->inner;
                isDeref = true;
            }
            if (auto* ptrTy = dynamic_cast<const PointerType*>(baseTy)) {
                baseTy = ptrTy->inner;
                isDeref = true;
            }

            if (auto* st = dynamic_cast<const StructType*>(baseTy)) {
                if (node.memberId != kInvalidSymbolID) {
                    const auto& sym = table.getSymbol(node.memberId);
                    if (sym.kind == SymbolKind::Field && sym.decl) {
                        const Type* fTy = typeTable[node.memberId];
                        
                        const StructDeclNode* structDecl = nullptr;
                        const auto& structSym = table.getSymbol(st->structSymbolId);
                        if (structSym.kind == SymbolKind::Struct && structSym.decl) {
                            structDecl = static_cast<const StructDeclNode*>(structSym.decl);
                        }
                        
                        if (structDecl) {
                            std::unordered_map<SymbolID, const Type*> subst;
                            for (size_t i = 0; i < structDecl->genericParams.size(); ++i) {
                                if (i < st->genericArgs.size()) {
                                    subst[structDecl->genericParams[i].symbolId] = st->genericArgs[i];
                                }
                            }
                            node.inferredType = ctx.substitute(fTy, subst);
                        } else {
                            node.inferredType = fTy;
                        }
                    } else {
                        node.inferredType = typeTable[node.memberId];
                    }
                } else {
                    node.inferredType = ctx.getUnknown();
                }
            } else if (auto* enumTy = dynamic_cast<const EnumType*>(baseTy)) {
                node.inferredType = ctx.getUnknown();
            } else {
                node.inferredType = ctx.getUnknown();
            }
        }"""

content = re.sub(
    r"(\s*)void visit\(MemberExpr& node\) override \{\s*node\.base->accept\(\*this\);\s*const Type\* baseTy = node\.base->inferredType;\s*if \(baseTy\) baseTy = ctx\.unificationTable\.deepResolve\(baseTy, ctx\);.*?else \{\s*node\.inferredType = ctx\.getUnknown\(\);\s*\}\s*\}",
    "\n" + replacement,
    content,
    flags=re.DOTALL
)

with open('src/MiddleEnd/TypeChecker.cpp', 'w') as f:
    f.write(content)
print("Patched MemberExpr")

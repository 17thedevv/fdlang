import re

with open('src/MiddleEnd/TypeChecker.cpp', 'r') as f:
    content = f.read()

replacement = """        void visit(StructInitExpr& node) override {
            const Type* structType = ctx.getUnknown();
            if (node.structId != kInvalidSymbolID) {
                const auto& sym = table.getSymbol(node.structId);
                if (sym.kind == SymbolKind::Struct && sym.decl) {
                    auto* structDecl = static_cast<const StructDeclNode*>(sym.decl);
                    if (!structDecl->genericParams.empty()) {
                        std::vector<const Type*> args;
                        for (size_t i = 0; i < structDecl->genericParams.size(); ++i) {
                            if (i < node.genericArgs.size()) {
                                TypePrePass pre(table, ctx, typeTable, methodResolver, implementedTraits, monoEngine);
                                args.push_back(pre.evaluateTypeNode(node.genericArgs[i].get()));
                            } else {
                                args.push_back(ctx.getInferenceVar(ctx.newVar()));
                            }
                        }
                        structType = ctx.getStructType(node.structId, args);
                    } else {
                        structType = typeTable[node.structId];
                    }
                } else {
                    structType = typeTable[node.structId];
                }
            }
            node.inferredType = structType;
            for (auto& field : node.fields) {
                if (field.value) field.value->accept(*this);
            }
            
            if (auto* st = dynamic_cast<const StructType*>(structType)) {
                if (node.structId != kInvalidSymbolID) {
                    const auto& sym = table.getSymbol(node.structId);
                    if (sym.kind == SymbolKind::Struct && sym.decl) {
                        auto* structDecl = static_cast<const StructDeclNode*>(sym.decl);
                        std::unordered_map<SymbolID, const Type*> subst;
                        for (size_t i = 0; i < structDecl->genericParams.size(); ++i) {
                            if (i < st->genericArgs.size()) {
                                subst[structDecl->genericParams[i].symbolId] = st->genericArgs[i];
                            }
                        }
                        
                        for (auto& field : node.fields) {
                            if (!field.value) continue;
                            auto it = st->fieldIndices.find(std::string(field.name));
                            if (it != st->fieldIndices.end()) {
                                SymbolID fId = structDecl->fields[it->second]->symbolId;
                                const Type* fTy = typeTable[fId];
                                const Type* instTy = ctx.substitute(fTy, subst);
                                constraints.push_back(Constraint(ConstraintKind::Equality, instTy, field.value->inferredType, "field type mismatch", node.loc));
                            }
                        }
                    }
                }
            }
        }"""

content = re.sub(
    r"(\s*)void visit\(StructInitExpr& node\) override \{\s*const Type\* structType = ctx\.getUnknown\(\);\s*if \(node\.structId != kInvalidSymbolID\) structType = typeTable\[node\.structId\];\s*node\.inferredType = structType;\s*for \(auto& field : node\.fields\) field\.value->accept\(\*this\);\s*\}",
    "\n" + replacement,
    content,
    flags=re.DOTALL
)

with open('src/MiddleEnd/TypeChecker.cpp', 'w') as f:
    f.write(content)
print("Patched StructInitExpr")

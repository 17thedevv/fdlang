import re

with open('src/MiddleEnd/TypeChecker.cpp', 'r') as f:
    content = f.read()

cg_visit = """        void visit(NamedTypeNode& node) override {
            printf("[DEBUG] TypePrePass::visit(NamedTypeNode) start\\n"); fflush(stdout);
            if (node.symbolId != kInvalidSymbolID) {
                const auto& sym = table.getSymbol(node.symbolId);
                std::vector<const Type*> args;
                bool hasGenericArg = false;
                for (auto& argNode : node.genericArgs) {
                    const Type* t = evaluateTypeNode(argNode.get());
                    args.push_back(t);
                    if (dynamic_cast<const GenericParamType*>(t) || dynamic_cast<const InferenceVarType*>(t)) {
                        hasGenericArg = true;
                    }
                }
                
                SymbolID specId = node.symbolId;
                printf("[DEBUG] TypePrePass::visit(NamedTypeNode) hasGenericArg=%d\\n", (int)hasGenericArg); fflush(stdout);
                if (!args.empty() && !hasGenericArg && monoEngine && sym.decl) {
                    if (sym.kind == SymbolKind::Struct) {
                        auto* structDecl = static_cast<const StructDeclNode*>(sym.decl);
                        printf("[DEBUG] Requesting struct specialization\\n"); fflush(stdout);
                        SymbolID newId = monoEngine->requestStructSpecialization(structDecl, args, node.loc);
                        if (newId != kInvalidSymbolID) {
                            specId = newId;
                            node.symbolId = newId;
                            args.clear(); // specialized struct has no generic args
                        }
                    } else if (sym.kind == SymbolKind::Enum) {
                        auto* enumDecl = static_cast<const EnumDeclNode*>(sym.decl);
                        printf("[DEBUG] Requesting enum specialization\\n"); fflush(stdout);
                        SymbolID newId = monoEngine->requestEnumSpecialization(enumDecl, args, node.loc);
                        if (newId != kInvalidSymbolID) {
                            specId = newId;
                            node.symbolId = newId;
                            args.clear(); // specialized enum has no generic args
                        }
                    }
                }
                
                printf("[DEBUG] Looking up finalSym for specId=%llu\\n", (unsigned long long)specId); fflush(stdout);
                const auto& finalSym = table.getSymbol(specId);
                printf("[DEBUG] Got finalSym kind=%d\\n", (int)finalSym.kind); fflush(stdout);
                if (finalSym.kind == SymbolKind::Struct) evaluatedType = ctx.getStructType(specId, args);
                else if (finalSym.kind == SymbolKind::Enum) evaluatedType = ctx.getEnumType(specId, args);
                else if (finalSym.kind == SymbolKind::GenericParam) {
                    evaluatedType = ctx.getGenericParamType(specId, finalSym.name.view());
                } else {
                    printf("[DEBUG] Looking up typeTable for specId=%llu, size=%zu\\n", (unsigned long long)specId, typeTable.size()); fflush(stdout);
                    evaluatedType = typeTable[specId]; 
                }
            } else {
                evaluatedType = ctx.getUnknown();
            }
            printf("[DEBUG] TypePrePass::visit(NamedTypeNode) end\\n"); fflush(stdout);
        }"""

content = re.sub(
    r"(\s*)void visit\(NamedTypeNode& node\) override \{\s*printf\(\"\[DEBUG\] TypePrePass::visit\(NamedTypeNode\)\\n\"\); fflush\(stdout\);.*?\}\s*\}",
    "\n" + cg_visit,
    content,
    flags=re.DOTALL
)

with open('src/MiddleEnd/TypeChecker.cpp', 'w') as f:
    f.write(content)
print("Added detailed prints to NamedTypeNode")

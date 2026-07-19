import re

with open('src/MiddleEnd/TypeChecker.cpp', 'r') as f:
    content = f.read()

cg_visit = """        void visit(NamedTypeNode& node) override {
            if (node.symbolId != kInvalidSymbolID) {
                const auto& sym = table.getSymbol(node.symbolId);
                std::vector<const Type*> args;
                for (auto& argNode : node.genericArgs) {
                    args.push_back(evaluateTypeNode(argNode.get()));
                }
                
                SymbolID specId = node.symbolId;
                if (!args.empty() && monoEngine && sym.decl) {
                    if (sym.kind == SymbolKind::Struct) {
                        auto* structDecl = static_cast<const StructDeclNode*>(sym.decl);
                        SymbolID newId = monoEngine->requestStructSpecialization(structDecl, args, node.loc);
                        if (newId != kInvalidSymbolID) {
                            specId = newId;
                            node.symbolId = newId;
                        }
                    } else if (sym.kind == SymbolKind::Enum) {
                        auto* enumDecl = static_cast<const EnumDeclNode*>(sym.decl);
                        SymbolID newId = monoEngine->requestEnumSpecialization(enumDecl, args, node.loc);
                        if (newId != kInvalidSymbolID) {
                            specId = newId;
                            node.symbolId = newId;
                        }
                    }
                }
                
                const auto& finalSym = table.getSymbol(specId);
                if (finalSym.kind == SymbolKind::Struct) evaluatedType = ctx.getStructType(specId, args);
                else if (finalSym.kind == SymbolKind::Enum) evaluatedType = ctx.getEnumType(specId, args);
                else if (finalSym.kind == SymbolKind::GenericParam) {
                    evaluatedType = ctx.getGenericParamType(specId, finalSym.name.view());
                } else {
                    evaluatedType = typeTable[specId]; 
                }
            } else {
                evaluatedType = ctx.getUnknown();
            }
        }"""

content = re.sub(
    r"(\s*)void visit\(NamedTypeNode& node\) override \{\s*if \(node\.symbolId != kInvalidSymbolID\) \{\s*const auto& sym = table\.getSymbol\(node\.symbolId\);\s*std::vector<const Type\*> args;\s*for \(auto& argNode : node\.genericArgs\) \{\s*args\.push_back\(evaluateTypeNode\(argNode\.get\(\)\)\);\s*\}\s*if \(sym\.kind == SymbolKind::Struct\) evaluatedType = ctx\.getStructType\(node\.symbolId, args\);\s*else if \(sym\.kind == SymbolKind::Enum\) evaluatedType = ctx\.getEnumType\(node\.symbolId, args\);\s*else if \(sym\.kind == SymbolKind::GenericParam\) \{\s*evaluatedType = ctx\.getGenericParamType\(node\.symbolId, sym\.name\.view\(\)\);\s*\} else \{\s*evaluatedType = typeTable\[node\.symbolId\]; \s*\}\s*\} else \{\s*evaluatedType = ctx\.getUnknown\(\);\s*\}\s*\}",
    "\n" + cg_visit,
    content
)

with open('src/MiddleEnd/TypeChecker.cpp', 'w') as f:
    f.write(content)
print("Patched TypePrePass::visit(NamedTypeNode)")

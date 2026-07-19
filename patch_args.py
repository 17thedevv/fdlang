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
                            args.clear(); // specialized struct has no generic args
                        }
                    } else if (sym.kind == SymbolKind::Enum) {
                        auto* enumDecl = static_cast<const EnumDeclNode*>(sym.decl);
                        SymbolID newId = monoEngine->requestEnumSpecialization(enumDecl, args, node.loc);
                        if (newId != kInvalidSymbolID) {
                            specId = newId;
                            node.symbolId = newId;
                            args.clear(); // specialized enum has no generic args
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
    r"(\s*)void visit\(NamedTypeNode& node\) override \{\s*if \(node\.symbolId != kInvalidSymbolID\) \{\s*const auto& sym = table\.getSymbol\(node\.symbolId\);\s*std::vector<const Type\*> args;\s*for \(auto& argNode : node\.genericArgs\) \{\s*args\.push_back\(evaluateTypeNode\(argNode\.get\(\)\)\);\s*\}\s*SymbolID specId = node\.symbolId;\s*if \(!args\.empty\(\) && monoEngine && sym\.decl\) \{\s*if \(sym\.kind == SymbolKind::Struct\) \{\s*auto\* structDecl = static_cast<const StructDeclNode\*>\(sym\.decl\);\s*SymbolID newId = monoEngine->requestStructSpecialization\(structDecl, args, node\.loc\);\s*if \(newId != kInvalidSymbolID\) \{\s*specId = newId;\s*node\.symbolId = newId;\s*\}\s*\} else if \(sym\.kind == SymbolKind::Enum\) \{\s*auto\* enumDecl = static_cast<const EnumDeclNode\*>\(sym\.decl\);\s*SymbolID newId = monoEngine->requestEnumSpecialization\(enumDecl, args, node\.loc\);\s*if \(newId != kInvalidSymbolID\) \{\s*specId = newId;\s*node\.symbolId = newId;\s*\}\s*\}\s*\}\s*const auto& finalSym = table\.getSymbol\(specId\);\s*if \(finalSym\.kind == SymbolKind::Struct\) evaluatedType = ctx\.getStructType\(specId, args\);\s*else if \(finalSym\.kind == SymbolKind::Enum\) evaluatedType = ctx\.getEnumType\(specId, args\);\s*else if \(finalSym\.kind == SymbolKind::GenericParam\) \{\s*evaluatedType = ctx\.getGenericParamType\(specId, finalSym\.name\.view\(\)\);\s*\} else \{\s*evaluatedType = typeTable\[specId\]; \s*\}\s*\} else \{\s*evaluatedType = ctx\.getUnknown\(\);\s*\}\s*\}",
    "\n" + cg_visit,
    content
)

with open('src/MiddleEnd/TypeChecker.cpp', 'w') as f:
    f.write(content)
print("Patched args.clear() in NamedTypeNode")

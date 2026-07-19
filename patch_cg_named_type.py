import re

with open('src/MiddleEnd/TypeChecker.cpp', 'r') as f:
    content = f.read()

replacement = """        void visit(NamedTypeNode& node) override {
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
                if (!args.empty() && !hasGenericArg && monoEngine && sym.decl) {
                    if (sym.kind == SymbolKind::Struct) {
                        auto* structDecl = static_cast<const StructDeclNode*>(sym.decl);
                        SymbolID newId = monoEngine->requestStructSpecialization(structDecl, args, node.loc);
                        if (newId != kInvalidSymbolID) {
                            specId = newId;
                            node.symbolId = newId;
                            args.clear();
                        }
                    } else if (sym.kind == SymbolKind::Enum) {
                        auto* enumDecl = static_cast<const EnumDeclNode*>(sym.decl);
                        SymbolID newId = monoEngine->requestEnumSpecialization(enumDecl, args, node.loc);
                        if (newId != kInvalidSymbolID) {
                            specId = newId;
                            node.symbolId = newId;
                            args.clear();
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
    r"(\s*)void visit\(BuiltinTypeNode& node\) override \{ evaluatedType = ctx\.getPrimitive\(node\.kind\); \}\s*void visit\(NamedTypeNode& node\) override \{.*?(?=void visit\(PointerTypeNode& node\) override \{)",
    r"\1void visit(BuiltinTypeNode& node) override { evaluatedType = ctx.getPrimitive(node.kind); }\n" + replacement + "\n          ",
    content,
    flags=re.DOTALL
)

with open('src/MiddleEnd/TypeChecker.cpp', 'w') as f:
    f.write(content)
print("Patched ConstraintGenerator::visit(NamedTypeNode)")

import re

with open('src/MiddleEnd/TypeChecker.cpp', 'r') as f:
    content = f.read()

# 1. Update ConstraintGenerator fields and constructor
content = re.sub(
    r"(class ConstraintGenerator.*?)(std::vector<const Type\*>& typeTable;\s*)(std::vector<Constraint>& constraints;\s*)(MethodResolver& methodResolver;\s*)(std::unordered_map<const Type\*, std::unordered_set<SymbolID>>& implementedTraits;\s*)",
    r"\1\2\3\4\5MonomorphizationEngine* monoEngine;\n        ",
    content,
    flags=re.DOTALL
)

content = re.sub(
    r"(ConstraintGenerator\(SymbolTable& table, DiagnosticEngine& diag, TypeContext& ctx, std::vector<const Type\*>& typeTable, std::vector<Constraint>& constraints, MethodResolver& methodResolver, std::unordered_map<const Type\*, std::unordered_set<SymbolID>>& implementedTraits)(.*?: table\(table\), diag\(diag\), ctx\(ctx\), typeTable\(typeTable\), constraints\(constraints\), methodResolver\(methodResolver\), implementedTraits\(implementedTraits\))",
    r"\1, MonomorphizationEngine* monoEngine\2, monoEngine(monoEngine)",
    content,
    flags=re.DOTALL
)

# 2. Update instantiation
content = re.sub(
    r"(ConstraintGenerator gen\(table_, diag_, ctx_, typeTable_, constraints, methodResolver_, implementedTraits_)\)",
    r"\1, monoEngine_)",
    content
)

# 3. Add to ConstraintGenerator::visit(StructInitExpr)
cg_visit = """        void visit(StructInitExpr& node) override {
            const Type* structType = ctx.getUnknown();
            if (node.structId != kInvalidSymbolID) {
                if (!node.genericArgs.empty()) {
                    std::vector<const Type*> args;
                    for (auto& argNode : node.genericArgs) {
                        args.push_back(evaluateTypeNode(argNode.get()));
                    }
                    structType = ctx.getStructType(node.structId, args);
                    
                    if (monoEngine) {
                        const auto& sym = table.getSymbol(node.structId);
                        if (sym.kind == SymbolKind::Struct && sym.decl) {
                            auto* structDecl = static_cast<const StructDeclNode*>(sym.decl);
                            SymbolID specId = monoEngine->requestStructSpecialization(structDecl, args, node.loc);
                            if (specId != kInvalidSymbolID) {
                                node.structId = specId;
                                structType = typeTable[specId];
                            }
                        }
                    }
                } else {
                    structType = typeTable[node.structId];
                }
            }
            node.inferredType = structType;
            for (auto& field : node.fields) field.value->accept(*this);
        }"""

content = re.sub(
    r"(\s*)void visit\(StructInitExpr& node\) override \{\s*const Type\* structType = ctx\.getUnknown\(\);\s*if \(node\.structId != kInvalidSymbolID\) \{\s*if \(!node\.genericArgs\.empty\(\)\) \{\s*std::vector<const Type\*> args;\s*for \(auto& argNode : node\.genericArgs\) \{\s*args\.push_back\(evaluateTypeNode\(argNode\.get\(\)\)\);\s*\}\s*structType = ctx\.getStructType\(node\.structId, args\);\s*\} else \{\s*structType = typeTable\[node\.structId\];\s*\}\s*\}\s*node\.inferredType = structType;\s*for \(auto& field : node\.fields\) field\.value->accept\(\*this\);\s*\}",
    "\n" + cg_visit + "\n",
    content
)


with open('src/MiddleEnd/TypeChecker.cpp', 'w') as f:
    f.write(content)
print("Patched ConstraintGenerator")

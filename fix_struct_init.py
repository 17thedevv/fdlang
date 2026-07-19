import sys

with open('src/MiddleEnd/TypeChecker.cpp', 'r') as f:
    content = f.read()

target = '''        void visit(StructInitExpr& node) override {
            const Type* structType = ctx.getUnknown();
            if (node.structId != kInvalidSymbolID) structType = typeTable[node.structId];
            node.inferredType = structType;
            for (auto& field : node.fields) field.value->accept(*this);
        }'''

replacement = '''        void visit(StructInitExpr& node) override {
            const Type* structType = ctx.getUnknown();
            if (node.structId != kInvalidSymbolID) {
                if (!node.genericArgs.empty()) {
                    std::vector<const Type*> args;
                    for (auto& argNode : node.genericArgs) {
                        args.push_back(evaluateTypeNode(argNode.get()));
                    }
                    structType = ctx.getStructType(node.structId, args);
                } else {
                    structType = typeTable[node.structId];
                }
            }
            node.inferredType = structType;
            for (auto& field : node.fields) field.value->accept(*this);
        }'''

if target in content:
    content = content.replace(target, replacement)
    print("Patched ConstraintGenerator::visit(StructInitExpr)")
else:
    print("Target not found for ConstraintGenerator")

# Now TypeResolver::visit(StructInitExpr)
target2 = '''        void visit(StructInitExpr& node) override {}'''
replacement2 = '''        void visit(StructInitExpr& node) override {
            resolve(node.inferredType, node.loc);
            if (!node.genericArgs.empty() && monoEngine) {
                std::vector<const Type*> concreteArgs;
                for (auto& arg : node.genericArgs) {
                    concreteArgs.push_back(ctx.unificationTable.deepResolve(evaluateTypeNode(arg.get()), ctx));
                }
                const auto& sym = table.getSymbol(node.structId);
                if (sym.kind == SymbolKind::Struct && sym.decl) {
                    auto* structDecl = static_cast<const StructDeclNode*>(sym.decl);
                    SymbolID specId = monoEngine->requestStructSpecialization(structDecl, concreteArgs, node.loc);
                    if (specId != kInvalidSymbolID) {
                        node.structId = specId;
                    }
                }
            }
            for (auto& field : node.fields) {
                if (field.value) field.value->accept(*this);
            }
        }'''

if target2 in content:
    content = content.replace(target2, replacement2)
    print("Patched TypeResolver::visit(StructInitExpr)")
else:
    print("Target2 not found")

with open('src/MiddleEnd/TypeChecker.cpp', 'w') as f:
    f.write(content)

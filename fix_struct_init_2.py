import sys

with open('src/MiddleEnd/TypeChecker.cpp', 'r') as f:
    content = f.read()

# Fix TypePrePass mistakenly replaced
bad_prepass = '''        void visit(StructInitExpr& node) override {
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
content = content.replace(bad_prepass, '        void visit(StructInitExpr& node) override {}', 1)

# Fix ConstraintGenerator evaluateTypeNode
# ConstraintGenerator has evaluateTypeNode? Wait, evaluateTypeNode is defined where?
# It's a helper method in TypeChecker class or a global function?
# Let's check where evaluateTypeNode is.

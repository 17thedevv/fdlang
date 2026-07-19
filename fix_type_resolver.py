import sys

with open('src/MiddleEnd/TypeChecker.cpp', 'r') as f:
    content = f.read()

target = '''        void visit(StructInitExpr& node) override {
            resolve(node.inferredType, node.loc);
            if (!node.genericArgs.empty() && monoEngine) {
                std::vector<const Type*> concreteArgs;
                for (auto& arg : node.genericArgs) {
                    concreteArgs.push_back(ctx.unificationTable.deepResolve(evaluateTypeNode(arg.get()), ctx));
                }'''

replacement = '''        void visit(StructInitExpr& node) override {
            resolve(node.inferredType, node.loc);
            if (!node.genericArgs.empty() && monoEngine) {
                std::vector<const Type*> concreteArgs;
                
                // We need to evaluate the type nodes using TypePrePass
                // But we don't have implementedTraits here. 
                // Wait, typeTable has the types if we visited them. But we didn't.
                // We can use the same trick as TypeResolver::resolveGenericsAndSpecialize
                
                for (auto& arg : node.genericArgs) {
                    // This assumes the arg was already evaluated by ConstraintGenerator
                    // where node.genericArgs[i] has an inferredType? No, TypeNode doesn't have inferredType.
                    
                    // Actually, let's just use evaluateTypeNode if we make a dummy PrePass
                    std::unordered_map<const Type*, std::unordered_set<SymbolID>> dummyTraits;
                    TypePrePass pre(table, ctx, typeTable, methodResolver, dummyTraits);
                    concreteArgs.push_back(ctx.unificationTable.deepResolve(pre.evaluateTypeNode(arg.get()), ctx));
                }'''

if target in content:
    content = content.replace(target, replacement)
    print("Patched TypeResolver::visit(StructInitExpr) evaluateTypeNode")
else:
    print("Target not found")

with open('src/MiddleEnd/TypeChecker.cpp', 'w') as f:
    f.write(content)

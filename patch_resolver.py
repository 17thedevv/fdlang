import sys

with open('src/MiddleEnd/TypeChecker.cpp', 'r') as f:
    content = f.read()

target = '''        void visit(ImplDeclNode& node) override {
            auto optSelfId = table.lookup(Identifier(std::string("Self")), node.bodyScopeId);
            const Type* selfType = nullptr;
            if (optSelfId != kInvalidSymbolID) {
                selfType = typeTable[optSelfId];
            }
            
            // Register generic Impl blocks for later monomorphization
            if (!node.genericParams.empty() && monoEngine) {
                SymbolID targetStructId = kInvalidSymbolID;
                if (auto* structTy = dynamic_cast<const StructType*>(selfType)) {
                    targetStructId = structTy->structSymbolId;
                } else if (auto* enumTy = dynamic_cast<const EnumType*>(selfType)) {
                    targetStructId = enumTy->enumSymbolId;
                }
                if (targetStructId != kInvalidSymbolID) {
                    monoEngine->registerGenericImpl(targetStructId, &node);
                }
            }'''

replacement = target + '''
            if (!node.genericParams.empty()) return;'''

if target in content:
    content = content.replace(target, replacement)
    print("Patched TypeResolver::visit(ImplDeclNode)")
else:
    print("Target not found for TypeResolver::visit(ImplDeclNode)")

target_fn = '''        void visit(FunctionDeclNode& node) override {
            if (node.body) node.body->accept(*this);
        }'''
replacement_fn = '''        void visit(FunctionDeclNode& node) override {
            if (!node.genericParams.empty()) return;
            if (node.body) node.body->accept(*this);
        }'''

if target_fn in content:
    content = content.replace(target_fn, replacement_fn)
    print("Patched TypeResolver::visit(FunctionDeclNode)")
else:
    print("Target not found for TypeResolver::visit(FunctionDeclNode)")

with open('src/MiddleEnd/TypeChecker.cpp', 'w') as f:
    f.write(content)

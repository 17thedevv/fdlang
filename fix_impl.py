import sys

with open('src/MiddleEnd/TypeChecker.cpp', 'r') as f:
    content = f.read()

wrong_target = '''        void visit(ImplDeclNode& node) override {
            const Type* selfType = evaluateTypeNode(node.selfType.get());
            if (!selfType) return;

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
            }

            auto optSelfId = table.lookup(Identifier(std::string("Self")), node.bodyScopeId);
            if (optSelfId) {
                typeTable[*optSelfId] = selfType;
            }'''

wrong_replacement = '''        void visit(ImplDeclNode& node) override {
            const Type* selfType = evaluateTypeNode(node.selfType.get());
            if (!selfType) return;

            auto optSelfId = table.lookup(Identifier(std::string("Self")), node.bodyScopeId);
            if (optSelfId) {
                typeTable[*optSelfId] = selfType;
            }'''

right_target = '''        void visit(ImplDeclNode& node) override { 
          sm.enterExistingScope(node.bodyScopeId);
          for (auto& gp : node.genericParams) {
              for (auto& bound : gp.bounds) {
                  bound->accept(static_cast<TypeVisitor&>(*this));
              }
          }
          node.selfType->accept(static_cast<TypeVisitor&>(*this));'''

right_replacement = '''        void visit(ImplDeclNode& node) override { 
          sm.enterExistingScope(node.bodyScopeId);
          
          if (!node.genericParams.empty() && monoEngine) {
              const Type* selfType = ctx.unificationTable.deepResolve(typeTable[node.selfType->symbolId], ctx);
              SymbolID targetStructId = kInvalidSymbolID;
              if (auto* structTy = dynamic_cast<const StructType*>(selfType)) {
                  targetStructId = structTy->structSymbolId;
              } else if (auto* enumTy = dynamic_cast<const EnumType*>(selfType)) {
                  targetStructId = enumTy->enumSymbolId;
              }
              if (targetStructId != kInvalidSymbolID) {
                  monoEngine->registerGenericImpl(targetStructId, &node);
              }
          }
          
          for (auto& gp : node.genericParams) {
              for (auto& bound : gp.bounds) {
                  bound->accept(static_cast<TypeVisitor&>(*this));
              }
          }
          node.selfType->accept(static_cast<TypeVisitor&>(*this));'''

content = content.replace(wrong_target, wrong_replacement)
content = content.replace(right_target, right_replacement)

with open('src/MiddleEnd/TypeChecker.cpp', 'w') as f:
    f.write(content)

with open('src/MiddleEnd/MonomorphizationEngine.cpp', 'r') as f:
    mono_content = f.read()
    
# Remove the bad lines at EOF
mono_content = mono_content.replace('''
void MonomorphizationEngine::registerGenericImpl(SymbolID targetStructId, const ImplDeclNode* implNode) {
    if (targetStructId != kInvalidSymbolID) {
        genericImpls[targetStructId].push_back(implNode);
    }
}''', '')

# Insert it before the last '}'
last_brace = mono_content.rfind('}')
mono_content = mono_content[:last_brace] + '''
void MonomorphizationEngine::registerGenericImpl(SymbolID targetStructId, const ImplDeclNode* implNode) {
    if (targetStructId != kInvalidSymbolID) {
        genericImpls[targetStructId].push_back(implNode);
    }
}
''' + mono_content[last_brace:]

with open('src/MiddleEnd/MonomorphizationEngine.cpp', 'w') as f:
    f.write(mono_content)

print("Patched successfully")

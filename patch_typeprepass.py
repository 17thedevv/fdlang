import re

with open('src/MiddleEnd/TypeChecker.cpp', 'r') as f:
    content = f.read()

content = re.sub(
    r"(\s*const Type\* evaluatedType = nullptr;)",
    r"\1\n          MonomorphizationEngine* monoEngine;",
    content
)

content = re.sub(
    r"TypePrePass\(SymbolTable& table, TypeContext& ctx, std::vector<const Type\*>& typeTable, MethodResolver& methodResolver, std::unordered_map<const Type\*, std::unordered_set<SymbolID>>& implementedTraits\)\s*: table\(table\), ctx\(ctx\), typeTable\(typeTable\), methodResolver\(methodResolver\), implementedTraits\(implementedTraits\) \{\}",
    r"TypePrePass(SymbolTable& table, TypeContext& ctx, std::vector<const Type*>& typeTable, MethodResolver& methodResolver, std::unordered_map<const Type*, std::unordered_set<SymbolID>>& implementedTraits, MonomorphizationEngine* monoEngine)\n              : table(table), ctx(ctx), typeTable(typeTable), methodResolver(methodResolver), implementedTraits(implementedTraits), monoEngine(monoEngine) {}",
    content
)

content = re.sub(
    r"TypePrePass pre\(table_, ctx_, typeTable_, methodResolver_, implementedTraits_\);",
    r"TypePrePass pre(table_, ctx_, typeTable_, methodResolver_, implementedTraits_, monoEngine_);",
    content
)

content = re.sub(
    r"TypePrePass pre\(table, ctx, typeTable, methodResolver, implementedTraits\);",
    r"TypePrePass pre(table, ctx, typeTable, methodResolver, implementedTraits, monoEngine);",
    content
)

impl_visit = """        void visit(ImplDeclNode& node) override {
            if (!node.genericParams.empty()) {
                if (monoEngine) {
                    if (node.selfType) {
                        const Type* t = evaluateTypeNode(node.selfType.get());
                        if (auto* st = dynamic_cast<const StructType*>(t)) {
                            monoEngine->registerGenericImpl(st->structSymbolId, &node);
                        }
                    }
                }
                return;
            }
            if (node.selfType) {
                const Type* st = evaluateTypeNode(node.selfType.get());
                const Type* tr = nullptr;
                if (node.traitType) {
                    tr = evaluateTypeNode(node.traitType.get());
                    if (auto* trTy = dynamic_cast<const TraitType*>(tr)) {
                        implementedTraits[st].insert(trTy->traitSymbolId);
                    }
                }
                for (auto& method : node.methods) {
                    methodResolver.registerMethod(st, std::string(method->name), method->symbolId);
                    method->accept(*this);
                }
            }
        }"""
content = re.sub(
    r"(\s*)void visit\(ImplDeclNode& node\) override \{\s*if \(node\.selfType\) \{\s*const Type\* st = evaluateTypeNode\(node\.selfType\.get\(\)\);\s*const Type\* tr = nullptr;\s*if \(node\.traitType\) \{\s*tr = evaluateTypeNode\(node\.traitType\.get\(\)\);\s*if \(auto\* trTy = dynamic_cast<const TraitType\*>\(tr\)\) \{\s*implementedTraits\[st\]\.insert\(trTy->traitSymbolId\);\s*\}\s*\}\s*for \(auto& method : node\.methods\) \{\s*methodResolver\.registerMethod\(st, std::string\(method->name\), method->symbolId\);\s*\}\s*\}\s*\}",
    "\n" + impl_visit,
    content,
    flags=re.DOTALL
)

with open('src/MiddleEnd/TypeChecker.cpp', 'w') as f:
    f.write(content)
print("Patched TypePrePass")

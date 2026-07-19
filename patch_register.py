import re

with open('src/MiddleEnd/TypeChecker.cpp', 'r') as f:
    content = f.read()

# Add MonomorphizationEngine* monoEngine to TypePrePass
content = re.sub(
    r"(class TypePrePass.*?)(const Type\* evaluatedType = nullptr;\s*)",
    r"\1\2MonomorphizationEngine* monoEngine;\n\n        ",
    content,
    flags=re.DOTALL
)

content = re.sub(
    r"(TypePrePass\(SymbolTable& table, TypeContext& ctx, std::vector<const Type\*>& typeTable, MethodResolver& methodResolver, std::unordered_map<const Type\*, std::unordered_set<SymbolID>>& implementedTraits)(\))",
    r"\1, MonomorphizationEngine* monoEngine\2",
    content,
    flags=re.DOTALL
)

content = re.sub(
    r"(: table\(table\), ctx\(ctx\), typeTable\(typeTable\), methodResolver\(methodResolver\), implementedTraits\(implementedTraits\))",
    r"\1, monoEngine(monoEngine)",
    content,
    flags=re.DOTALL
)

# Call registerGenericImpl inside TypePrePass::visit(ImplDeclNode)
cg_visit = """        void visit(ImplDeclNode& node) override {
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
            }"""

content = re.sub(
    r"(\s*)void visit\(ImplDeclNode& node\) override \{\s*printf\(\"\[DEBUG\] TypePrePass::visit\(ImplDeclNode\) genericParams size=%zu\\n\", node\.genericParams\.size\(\)\);\s*if \(!node\.genericParams\.empty\(\)\) return;",
    "\n" + cg_visit,
    content
)

# Update TypePrePass instantiation in TypeChecker::check
content = re.sub(
    r"TypePrePass pre\(table_, ctx_, typeTable_, methodResolver_, implementedTraits_\);",
    r"TypePrePass pre(table_, ctx_, typeTable_, methodResolver_, implementedTraits_, monoEngine_);",
    content
)
# Update TypePrePass instantiation in TypeResolver::visit(StructInitExpr)
content = re.sub(
    r"TypePrePass pre\(table, ctx, typeTable, methodResolver, dummyTraits\);",
    r"TypePrePass pre(table, ctx, typeTable, methodResolver, dummyTraits, monoEngine);",
    content
)

with open('src/MiddleEnd/TypeChecker.cpp', 'w') as f:
    f.write(content)
print("Patched TypePrePass registerGenericImpl")

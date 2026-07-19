import re

with open('src/MiddleEnd/TypeChecker.cpp', 'r') as f:
    content = f.read()

# 1. In TypePrePass, modify evaluateTypeNode to be a public method so ConstraintGenerator can call it on the pre object.
# Wait, evaluateTypeNode is ALREADY public because it's under public:!
# Let's verify this.
# Let's just fix TypeResolver::visit(ImplDeclNode) to register generic Impls.
type_resolver_impl = r"(class TypeResolver\b.*?void visit\(ImplDeclNode& node\) override \{.*?)(auto optSelfId = table\.lookup\(Identifier\(std::string\(\"Self\"\)\), node\.bodyScopeId\);)"

def repl_impl(m):
    return m.group(1) + '''// Register generic Impl blocks for later monomorphization
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
            
            ''' + m.group(2)

content = re.sub(type_resolver_impl, repl_impl, content, flags=re.DOTALL)

# 2. In ConstraintGenerator, fix StructInitExpr to evaluate generic args.
cg_struct_init = r"(class ConstraintGenerator\b.*?void visit\(StructInitExpr& node\) override \{.*?)(if \(node\.structId != kInvalidSymbolID\) structType = typeTable\[node\.structId\];)"

def repl_cg_struct(m):
    return m.group(1) + '''if (node.structId != kInvalidSymbolID) {
                if (!node.genericArgs.empty()) {
                    std::vector<const Type*> args;
                    for (auto& argNode : node.genericArgs) {
                        args.push_back(evaluateTypeNode(argNode.get()));
                    }
                    structType = ctx.getStructType(node.structId, args);
                } else {
                    structType = typeTable[node.structId];
                }
            }'''

content = re.sub(cg_struct_init, repl_cg_struct, content, flags=re.DOTALL)

# 3. In TypeResolver, fix StructInitExpr to request struct specialization.
tr_struct_init = r"(class TypeResolver\b.*?)(void visit\(StructInitExpr& node\) override \{\})"

def repl_tr_struct(m):
    return m.group(1) + '''void visit(StructInitExpr& node) override {
            resolve(node.inferredType, node.loc);
            if (!node.genericArgs.empty() && monoEngine) {
                std::vector<const Type*> concreteArgs;
                std::unordered_map<const Type*, std::unordered_set<SymbolID>> dummyTraits;
                TypePrePass pre(table, ctx, typeTable, methodResolver, dummyTraits);
                
                for (auto& arg : node.genericArgs) {
                    concreteArgs.push_back(ctx.unificationTable.deepResolve(pre.evaluateTypeNode(arg.get()), ctx));
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

content = re.sub(tr_struct_init, repl_tr_struct, content, flags=re.DOTALL)

with open('src/MiddleEnd/TypeChecker.cpp', 'w') as f:
    f.write(content)
print("Regex patch applied")

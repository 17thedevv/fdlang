import re

with open('src/MiddleEnd/TypeChecker.cpp', 'r') as f:
    content = f.read()

replacement = """                    } else if (c.kind == ConstraintKind::Field) {
                        const Type* objType = ctx.unificationTable.deepResolve(c.expected, ctx);
                        if (auto* ptr = dynamic_cast<const PointerType*>(objType)) objType = ptr->pointee;
                        if (auto* ref = dynamic_cast<const ReferenceType*>(objType)) objType = ref->pointee;
                        if (auto* st = dynamic_cast<const StructType*>(objType)) {
                            const auto& sym = table.getSymbol(st->structSymbolId);
                            if (sym.kind == SymbolKind::Struct && sym.decl) {
                                auto* structDecl = static_cast<StructDeclNode*>(sym.decl);
                                for (auto& field : structDecl->fields) {
                                    if (field->name == c.name) {
                                        if (field->symbolId != kInvalidSymbolID) {
                                            const Type* fTy = typeTable[field->symbolId];
                                            std::unordered_map<SymbolID, const Type*> subst;
                                            for (size_t j = 0; j < structDecl->genericParams.size(); ++j) {
                                                if (j < st->genericArgs.size()) {
                                                    subst[structDecl->genericParams[j].symbolId] = st->genericArgs[j];
                                                }
                                            }
                                            fTy = ctx.substitute(fTy, subst);
                                            if (unify(c.actual, fTy, c.loc)) changed = true;
                                        }
                                    }
                                }
                            }
                        }"""

content = re.sub(
    r"(\s*)\} else if \(c\.kind == ConstraintKind::Field\) \{\s*const Type\* objType = ctx\.unificationTable\.deepResolve\(c\.expected, ctx\);\s*if \(auto\* st = dynamic_cast<const StructType\*>\(objType\)\) \{\s*const auto& sym = table\.getSymbol\(st->structSymbolId\);\s*if \(sym\.kind == SymbolKind::Struct && sym\.decl\) \{\s*auto\* structDecl = static_cast<StructDeclNode\*>\(sym\.decl\);\s*for \(auto& field : structDecl->fields\) \{\s*if \(field->name == c\.name\) \{\s*if \(field->symbolId != kInvalidSymbolID\) \{\s*if \(unify\(c\.actual, typeTable\[field->symbolId\], c\.loc\)\) changed = true;\s*\}\s*\}\s*\}\s*\}\s*\}",
    "\n" + replacement,
    content,
    flags=re.DOTALL
)

with open('src/MiddleEnd/TypeChecker.cpp', 'w') as f:
    f.write(content)
print("Patched ConstraintKind::Field")

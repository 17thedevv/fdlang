
import re

with open("src/MiddleEnd/TypeChecker.cpp", "r") as f:
    content = f.read()

start_idx = content.find("if (auto* variantFn = dynamic_cast<const FunctionType*>(typeTable[variantId])) {")
end_idx = content.find("} else if (typeTable[variantId]->getKind() == TypeKind::Enum) {", start_idx)

old_code = content[start_idx:end_idx]

new_code = """if (auto* variantFn = dynamic_cast<const FunctionType*>(typeTable[variantId])) {
                                        if (patNode->fields.size() != variantFn->paramTypes.size()) {
                                            diag.error(c.loc, "Variant '" + sym.name.str() + "' requires " + std::to_string(variantFn->paramTypes.size()) + " fields, but " + std::to_string(patNode->fields.size()) + " were provided");
                                        } else {
                                            std::unordered_map<SymbolID, const Type*> substitutionMap;
                                            if (auto* enumType = dynamic_cast<const EnumType*>(variantFn->returnType)) {
                                                const auto& enumSym = table.getSymbol(enumType->enumSymbolId);
                                                if (enumSym.kind == SymbolKind::Enum && enumSym.decl) {
                                                    auto* enumDecl = static_cast<EnumDeclNode*>(enumSym.decl);
                                                    if (auto* actualEnumTy = dynamic_cast<const EnumType*>(c.expected)) {
                                                        for (size_t i = 0; i < enumDecl->genericParams.size() && i < actualEnumTy->genericArgs.size(); ++i) {
                                                            substitutionMap[enumDecl->genericParams[i].symbolId] = actualEnumTy->genericArgs[i];
                                                        }
                                                    }
                                                }
                                            }
                                            
                                            for (size_t i = 0; i < patNode->fields.size(); ++i) {
                                                const Type* fieldExpected = variantFn->paramTypes[i];
                                                if (!substitutionMap.empty()) {
                                                    fieldExpected = ctx.substitute(fieldExpected, substitutionMap);
                                                }
                                                PatternConstraintVisitor fieldVisitor(ctx, typeTable, constraints, fieldExpected);
                                                patNode->fields[i]->accept(fieldVisitor);
                                            }
                                        }
                                    """
                                    
content = content[:start_idx] + new_code + content[end_idx:]

with open("src/MiddleEnd/TypeChecker.cpp", "w") as f:
    f.write(content)

print("Rewritten PatternConstraintVisitor for Enum variants!")

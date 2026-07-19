
import re

with open("src/MiddleEnd/TypeChecker.cpp", "r") as f:
    content = f.read()

start_idx = content.find("if (auto* variantFn = dynamic_cast<const FunctionType*>(typeTable[variantId])) {")
end_idx = content.find("} else if (typeTable[variantId]->getKind() == TypeKind::Enum) {", start_idx)

new_code = """if (auto* variantFn = dynamic_cast<const FunctionType*>(typeTable[variantId])) {
                                        if (patNode->fields.size() != variantFn->paramTypes.size()) {
                                            diag.error(c.loc, "Variant '" + sym.name.str() + "' requires " + std::to_string(variantFn->paramTypes.size()) + " fields, but " + std::to_string(patNode->fields.size()) + " were provided");
                                        } else {
                                            std::unordered_map<SymbolID, const Type*> substitutionMap;
                                            std::vector<const Type*> freshArgs;
                                            const EnumType* enumType = dynamic_cast<const EnumType*>(variantFn->returnType);
                                            if (enumType) {
                                                const auto& enumSym = table.getSymbol(enumType->enumSymbolId);
                                                if (enumSym.kind == SymbolKind::Enum && enumSym.decl) {
                                                    auto* enumDecl = static_cast<EnumDeclNode*>(enumSym.decl);
                                                    for (const auto& param : enumDecl->genericParams) {
                                                        const Type* freshVar = ctx.getInferenceVar(ctx.newVar());
                                                        substitutionMap[param.symbolId] = freshVar;
                                                        freshArgs.push_back(freshVar);
                                                    }
                                                }
                                            }
                                            
                                            // Constrain c.expected to be the specialized EnumType
                                            if (enumType) {
                                                const Type* specializedEnum = freshArgs.empty() ? enumType : ctx.getEnumType(enumType->enumSymbolId, std::move(freshArgs));
                                                constraints.push_back(Constraint(ConstraintKind::Equality, c.expected, specializedEnum, "", c.loc));
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

print("Rewritten PatternConstraintVisitor to instantiate TypeVars!")

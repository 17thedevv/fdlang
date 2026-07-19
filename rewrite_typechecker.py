
import re

with open("src/MiddleEnd/TypeChecker.cpp", "r") as f:
    content = f.read()

# Replace EnumDeclNode visit in TypePrePass
start_idx = content.find("void visit(EnumDeclNode& node) override {")
end_idx = content.find("}", start_idx) + 1
while content[end_idx] != "}":
    end_idx = content.find("}", end_idx) + 1

new_visit = """void visit(EnumDeclNode& node) override {
            const Type* enumTy = ctx.getEnumType(node.symbolId);
            typeTable[node.symbolId] = enumTy;
            for (auto& variant : node.variants) {
                if (variant->symbolId != kInvalidSymbolID) {
                    if (variant->fields.empty()) {
                        typeTable[variant->symbolId] = enumTy;
                    } else {
                        std::vector<const Type*> fieldTypes;
                        std::vector<std::string> paramNames;
                        for (auto& field : variant->fields) {
                            fieldTypes.push_back(evaluateTypeNode(field->type.get()));
                            paramNames.push_back(std::string(field->name));
                        }
                        typeTable[variant->symbolId] = ctx.getFunctionType(std::move(paramNames), std::move(fieldTypes), enumTy, false);
                    }
                }
            }
        }"""
content = content[:start_idx] + new_visit + content[end_idx:]

# Replace Variant TupleType extraction in ConstraintGenerator
pat_start = content.find("// Get variant's expected tuple type from typeTable")
pat_end = content.find("changed = true;", pat_start)

new_pat = """// Get variant's expected function type or enum type from typeTable
                                    if (auto* variantFn = dynamic_cast<const FunctionType*>(typeTable[variantId])) {
                                        if (patNode->fields.size() != variantFn->paramTypes.size()) {
                                            diag.error(c.loc, "Variant '" + sym.name.str() + "' requires " + std::to_string(variantFn->paramTypes.size()) + " fields, but " + std::to_string(patNode->fields.size()) + " were provided");
                                        } else {
                                            for (size_t i = 0; i < patNode->fields.size(); ++i) {
                                                const Type* fieldExpected = variantFn->paramTypes[i];
                                                PatternConstraintVisitor fieldVisitor(ctx, typeTable, constraints, fieldExpected);
                                                patNode->fields[i]->accept(fieldVisitor);
                                            }
                                        }
                                    } else if (typeTable[variantId]->getKind() == TypeKind::Enum) {
                                        if (!patNode->fields.empty()) {
                                            diag.error(c.loc, "Variant '" + sym.name.str() + "' does not take any fields");
                                        }
                                    }
                                    """
content = content[:pat_start] + new_pat + content[pat_end:]

with open("src/MiddleEnd/TypeChecker.cpp", "w") as f:
    f.write(content)

print("Rewritten TypeChecker!")

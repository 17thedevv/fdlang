
import re

with open("src/MiddleEnd/TypeChecker.cpp", "r") as f:
    content = f.read()

start_idx = content.find("if (sym.kind == SymbolKind::Function && sym.decl) {")
end_idx = content.find("const Type* expectedFnType = ctx.getFunctionType", start_idx)

old_code = content[start_idx:end_idx]

new_code = """if (sym.kind == SymbolKind::Function && sym.decl) {
                        auto* fnDecl = static_cast<FunctionDeclNode*>(sym.decl);
                        if (!fnDecl->genericParams.empty()) {
                            if (!ident->genericArgs.empty()) {
                                for (auto& argNode : ident->genericArgs) {
                                    node.inferredGenericArgs.push_back(evaluateTypeNode(argNode.get()));
                                }
                            } else {
                                for (size_t i = 0; i < fnDecl->genericParams.size(); ++i) {
                                    node.inferredGenericArgs.push_back(ctx.getInferenceVar(ctx.newVar()));
                                }
                            }
                            
                            std::unordered_map<SymbolID, const Type*> substitutionMap;
                            for (size_t i = 0; i < fnDecl->genericParams.size() && i < node.inferredGenericArgs.size(); ++i) {
                                substitutionMap[fnDecl->genericParams[i].symbolId] = node.inferredGenericArgs[i];
                            }
                            node.callee->inferredType = ctx.substitute(node.callee->inferredType, substitutionMap);
                        }
                    } else if (sym.kind == SymbolKind::EnumVariant) {
                        if (!ident->genericArgs.empty() || true) {
                            if (auto* fnType = dynamic_cast<const FunctionType*>(node.callee->inferredType)) {
                                if (auto* enumType = dynamic_cast<const EnumType*>(fnType->returnType)) {
                                    const auto& enumSym = table.getSymbol(enumType->enumSymbolId);
                                    if (enumSym.kind == SymbolKind::Enum && enumSym.decl) {
                                        auto* enumDecl = static_cast<EnumDeclNode*>(enumSym.decl);
                                        if (!enumDecl->genericParams.empty()) {
                                            if (!ident->genericArgs.empty()) {
                                                for (auto& argNode : ident->genericArgs) {
                                                    node.inferredGenericArgs.push_back(evaluateTypeNode(argNode.get()));
                                                }
                                            } else {
                                                for (size_t i = 0; i < enumDecl->genericParams.size(); ++i) {
                                                    node.inferredGenericArgs.push_back(ctx.getInferenceVar(ctx.newVar()));
                                                }
                                            }
                                            std::unordered_map<SymbolID, const Type*> substitutionMap;
                                            for (size_t i = 0; i < enumDecl->genericParams.size() && i < node.inferredGenericArgs.size(); ++i) {
                                                substitutionMap[enumDecl->genericParams[i].symbolId] = node.inferredGenericArgs[i];
                                            }
                                            node.callee->inferredType = ctx.substitute(node.callee->inferredType, substitutionMap);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            """
            
content = content[:start_idx] + new_code + content[end_idx:]

with open("src/MiddleEnd/TypeChecker.cpp", "w") as f:
    f.write(content)

print("Rewritten CallExpr in TypeChecker!")

import sys

with open('src/MiddleEnd/TypeChecker.cpp', 'r') as f:
    content = f.read()

target = '''                        structType = ctx.getStructType(node.structId, args);
                    } else {'''

replacement = '''                        bool allConcrete = true;
                        for (auto* a : args) {
                            if (a->getKind() == TypeKind::InferenceVar || dynamic_cast<const GenericParamType*>(a)) {
                                allConcrete = false; break;
                            }
                        }
                        if (allConcrete && monoEngine) {
                            SymbolID specId = monoEngine->requestStructSpecialization(structDecl, args, node.loc);
                            if (specId != kInvalidSymbolID) {
                                node.structId = specId;
                                args.clear();
                                node.genericArgs.clear();
                            }
                        }
                        structType = ctx.getStructType(node.structId, args);
                    } else {'''

if target in content:
    content = content.replace(target, replacement)
    with open('src/MiddleEnd/TypeChecker.cpp', 'w') as f:
        f.write(content)
    print("Patched StructInitExpr 2 successfully")
else:
    print("Target not found")

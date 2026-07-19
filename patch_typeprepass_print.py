import sys

with open('src/MiddleEnd/TypeChecker.cpp', 'r') as f:
    content = f.read()

target = '''            for (auto& method : node.methods) {
                method->accept(*this);
                if (method->symbolId != kInvalidSymbolID) {
                    if (auto* fnTy = dynamic_cast<const FunctionType*>(typeTable[method->symbolId])) {
                        methodResolver.addMethod(selfType, std::string(method->name), method->symbolId, fnTy, ctx);
                    }
                }
            }'''

replacement = '''            for (auto& method : node.methods) {
                printf("[DEBUG TypePrePass] visiting method: %s, symbolId=%u\n", std::string(method->name).c_str(), method->symbolId);
                method->accept(*this);
                printf("[DEBUG TypePrePass] after accept method: %s, symbolId=%u\n", std::string(method->name).c_str(), method->symbolId);
                if (method->symbolId != kInvalidSymbolID) {
                    if (auto* fnTy = dynamic_cast<const FunctionType*>(typeTable[method->symbolId])) {
                        printf("[DEBUG TypePrePass] fnTy is valid, calling addMethod\n");
                        methodResolver.addMethod(selfType, std::string(method->name), method->symbolId, fnTy, ctx);
                    } else {
                        printf("[DEBUG TypePrePass] fnTy is null for method %s\n", std::string(method->name).c_str());
                    }
                } else {
                    printf("[DEBUG TypePrePass] method %s has kInvalidSymbolID\n", std::string(method->name).c_str());
                }
            }'''

if target in content:
    content = content.replace(target, replacement)
    with open('src/MiddleEnd/TypeChecker.cpp', 'w') as f:
        f.write(content)
    print("Patched TypePrePass ImplDeclNode successfully")
else:
    print("Target not found")

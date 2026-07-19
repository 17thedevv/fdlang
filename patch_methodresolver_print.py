import sys

with open('src/MiddleEnd/TypeChecker.cpp', 'r') as f:
    content = f.read()

target1 = '''void TypeChecker::MethodResolver::addMethod(const Type* receiverType, const std::string& name, SymbolID methodId, const FunctionType* type, TypeContext& ctx) {
    printf("[DEBUG MethodResolver::addMethod] receiverType=%p (%s) method=%s methodId=%u\n", (void*)receiverType, receiverType->toString().c_str(), name.c_str(), methodId);
    std::vector<const Type*> newParamTypes;'''

replacement1 = '''void TypeChecker::MethodResolver::addMethod(const Type* receiverType, const std::string& name, SymbolID methodId, const FunctionType* type, TypeContext& ctx) {
    std::vector<const Type*> newParamTypes;'''

target2 = '''std::optional<SymbolID> TypeChecker::MethodResolver::probe(const Type* receiverType, const std::string& name) {
    printf("[DEBUG MethodResolver::probe] receiverType=%p (%s) method=%s\n", (void*)receiverType, receiverType->toString().c_str(), name.c_str());
    if (auto it = implMap.find(receiverType); it != implMap.end()) {
        auto methodsIt = it->second.find(name);
        if (methodsIt != it->second.end() && !methodsIt->second.empty()) {
            printf("[DEBUG MethodResolver::probe] FOUND! methodId=%u\n", methodsIt->second[0].first);
            return methodsIt->second[0].first;
        }
    }
    printf("[DEBUG MethodResolver::probe] NOT FOUND\n");
    return std::nullopt;
}'''

replacement2 = '''std::optional<SymbolID> TypeChecker::MethodResolver::probe(const Type* receiverType, const std::string& name) {
    if (auto it = implMap.find(receiverType); it != implMap.end()) {
        auto methodsIt = it->second.find(name);
        if (methodsIt != it->second.end() && !methodsIt->second.empty()) {
            return methodsIt->second[0].first;
        }
    }
    return std::nullopt;
}'''

if target1 in content:
    content = content.replace(target1, replacement1)
    print("Patched target1")
else:
    print("Target1 not found")

if target2 in content:
    content = content.replace(target2, replacement2)
    print("Patched target2")
else:
    print("Target2 not found")

with open('src/MiddleEnd/TypeChecker.cpp', 'w') as f:
    f.write(content)


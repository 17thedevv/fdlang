import sys

with open('src/MiddleEnd/MonomorphizationEngine.cpp', 'r') as f:
    content = f.read()

target = '''    SymbolID newId = specializedAST->symbolId;
    insertedIt->second = newId;
    inProgress.erase(stableMangledName);
    specializedASTs.push_back(std::move(specializedAST));

    currentDepth--;
    return newId;'''

replacement = '''    SymbolID newId = specializedAST->symbolId;
    insertedIt->second = newId;
    inProgress.erase(stableMangledName);
    specializedASTs.push_back(std::move(specializedAST));

    // Specialize associated Impl blocks
    auto implIt = genericImpls.find(genericTemplate->symbolId);
    if (implIt != genericImpls.end()) {
        for (const auto* implNode : implIt->second) {
            auto specializedImpl = implNode->cloneAs<ImplDeclNode>();
            specializedImpl->genericParams.clear(); // Now concrete!
            
            std::unordered_map<std::string, std::unique_ptr<TypeNode>> implSubs;
            for (size_t i = 0; i < implNode->genericParams.size() && i < genericArgs.size(); ++i) {
                implSubs[std::string(implNode->genericParams[i].name)] = typeToAST(genericArgs[i], symTable);
            }
            
            SubstitutionVisitor implVisitor(std::move(implSubs));
            implVisitor.substitute(*specializedImpl);
            
            resolver.resolve(specializedImpl.get());
            typeChecker.check(specializedImpl.get());
            
            specializedASTs.push_back(std::move(specializedImpl));
        }
    }

    currentDepth--;
    return newId;'''

if target in content:
    content = content.replace(target, replacement, 1) # Only replace the first one (struct)
    with open('src/MiddleEnd/MonomorphizationEngine.cpp', 'w') as f:
        f.write(content)
    print("Patched successfully")
else:
    print("Target not found")


import re

with open("src/MiddleEnd/TypeChecker.cpp", "r") as f:
    content = f.read()

old_code = """        void visit(EnumDeclNode& node) override {
            const Type* enumTy = ctx.getEnumType(node.symbolId);
            typeTable[node.symbolId] = enumTy;"""
            
new_code = """        void visit(EnumDeclNode& node) override {
            std::vector<const Type*> genericArgs;
            for (auto& param : node.genericParams) {
                genericArgs.push_back(ctx.getGenericParamType(param.symbolId, param.name.view()));
                if (param.symbolId != kInvalidSymbolID) typeTable[param.symbolId] = genericArgs.back();
            }
            const Type* enumTy = genericArgs.empty() ? ctx.getEnumType(node.symbolId) : ctx.getEnumType(node.symbolId, genericArgs);
            typeTable[node.symbolId] = enumTy;"""

content = content.replace(old_code, new_code)

with open("src/MiddleEnd/TypeChecker.cpp", "w") as f:
    f.write(content)

print("Rewritten EnumDeclNode in TypeChecker!")

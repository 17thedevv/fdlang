import sys

with open('src/MiddleEnd/MonomorphizationEngine.cpp', 'r') as f:
    content = f.read()

content = content.replace("SymbolID MonomorphizationEngine::requestStructSpecialization(", "SymbolID MonomorphizationEngine::requestStructSpecialization_Trace(")

trace_func = '''
SymbolID MonomorphizationEngine::requestStructSpecialization(const StructDeclNode* genericTemplate, const std::vector<const Type*>& genericArgs, SourceLocation loc) {
    printf("[DEBUG] requestStructSpecialization: %s\\n", genericTemplate->name.c_str()); fflush(stdout);
    return requestStructSpecialization_Trace(genericTemplate, genericArgs, loc);
}
'''

content += "\n" + trace_func + "\n"

with open('src/MiddleEnd/MonomorphizationEngine.cpp', 'w') as f:
    f.write(content)

with open('src/MiddleEnd/TypeChecker.cpp', 'r') as f:
    content = f.read()

content = content.replace("bool check(ASTNode* root) {", "bool check(ASTNode* root) { static int d=0; d++; printf(\"[DEBUG] check depth=%d\\n\", d); fflush(stdout); ")

with open('src/MiddleEnd/TypeChecker.cpp', 'w') as f:
    f.write(content)

print("Injected tracing successfully")

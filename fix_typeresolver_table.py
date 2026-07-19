import sys
with open('src/MiddleEnd/TypeChecker.cpp', 'r') as f:
    content = f.read()

target1 = '''        TypeResolver(SymbolTable& table, DiagnosticEngine& diag, TypeContext& ctx, MonomorphizationEngine* monoEngine, MethodResolver& methodResolver) 
            : table(table), diag(diag), ctx(ctx), monoEngine(monoEngine), methodResolver(methodResolver) {}'''
replacement1 = '''        std::vector<const Type*>& typeTable;
        TypeResolver(SymbolTable& table, DiagnosticEngine& diag, TypeContext& ctx, MonomorphizationEngine* monoEngine, MethodResolver& methodResolver, std::vector<const Type*>& typeTable) 
            : table(table), diag(diag), ctx(ctx), monoEngine(monoEngine), methodResolver(methodResolver), typeTable(typeTable) {}'''

target2 = '''    TypeResolver resolver(table_, diag_, ctx_, monoEngine_, methodResolver_);'''
replacement2 = '''    TypeResolver resolver(table_, diag_, ctx_, monoEngine_, methodResolver_, typeTable_);'''

content = content.replace(target1, replacement1)
content = content.replace(target2, replacement2)

with open('src/MiddleEnd/TypeChecker.cpp', 'w') as f:
    f.write(content)

import sys
import re

with open('src/MiddleEnd/TypeChecker.cpp', 'r') as f:
    content = f.read()

# Fix TypeResolver constructor
pat = r"(class TypeResolver : public ASTVisitor \{.*?MethodResolver& methodResolver;\n)(\s*)(public:\n\s*TypeResolver\(SymbolTable& table, DiagnosticEngine& diag, TypeContext& ctx, MonomorphizationEngine\* monoEngine, MethodResolver& methodResolver\)\s*:\s*table\(table\), diag\(diag\), ctx\(ctx\), monoEngine\(monoEngine\), methodResolver\(methodResolver\) \{\})"

def repl(m):
    return m.group(1) + "        std::vector<const Type*>& typeTable;\n" + m.group(2) + '''public:
            TypeResolver(SymbolTable& table, DiagnosticEngine& diag, TypeContext& ctx, MonomorphizationEngine* monoEngine, MethodResolver& methodResolver, std::vector<const Type*>& typeTable) 
                : table(table), diag(diag), ctx(ctx), monoEngine(monoEngine), methodResolver(methodResolver), typeTable(typeTable) {}'''

content = re.sub(pat, repl, content, flags=re.DOTALL)

# Fix TypeResolver instantiation
pat2 = r"TypeResolver resolver\(table_, diag_, ctx_, monoEngine_, methodResolver_\);"
repl2 = r"TypeResolver resolver(table_, diag_, ctx_, monoEngine_, methodResolver_, typeTable_);"
content = re.sub(pat2, repl2, content)

with open('src/MiddleEnd/TypeChecker.cpp', 'w') as f:
    f.write(content)
print("Regex patch applied for TypeResolver constructor")

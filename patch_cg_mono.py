import re

with open('src/MiddleEnd/TypeChecker.cpp', 'r') as f:
    content = f.read()

# Add monoEngine field to ConstraintGenerator
content = re.sub(
    r"(\s*const Type\* currentReturnType = nullptr;)",
    r"\1\n          MonomorphizationEngine* monoEngine;",
    content
)

# Update ConstraintGenerator constructor
content = re.sub(
    r"ConstraintGenerator\(SymbolTable& table, DiagnosticEngine& diag, TypeContext& ctx, std::vector<const Type\*>& typeTable, std::vector<Constraint>& constraints, MethodResolver& methodResolver, std::unordered_map<const Type\*, std::unordered_set<SymbolID>>& implementedTraits\)\s*: table\(table\), diag\(diag\), ctx\(ctx\), typeTable\(typeTable\), constraints\(constraints\), methodResolver\(methodResolver\), implementedTraits\(implementedTraits\) \{\}",
    r"ConstraintGenerator(SymbolTable& table, DiagnosticEngine& diag, TypeContext& ctx, std::vector<const Type*>& typeTable, std::vector<Constraint>& constraints, MethodResolver& methodResolver, std::unordered_map<const Type*, std::unordered_set<SymbolID>>& implementedTraits, MonomorphizationEngine* monoEngine)\n              : table(table), diag(diag), ctx(ctx), typeTable(typeTable), constraints(constraints), methodResolver(methodResolver), implementedTraits(implementedTraits), monoEngine(monoEngine) {}",
    content
)

# Update ConstraintGenerator instantiation
content = re.sub(
    r"ConstraintGenerator gen\(table_, diag_, ctx_, typeTable_, constraints, methodResolver_, implementedTraits_\);",
    r"ConstraintGenerator gen(table_, diag_, ctx_, typeTable_, constraints, methodResolver_, implementedTraits_, monoEngine_);",
    content
)

with open('src/MiddleEnd/TypeChecker.cpp', 'w') as f:
    f.write(content)
print("Patched ConstraintGenerator monoEngine")

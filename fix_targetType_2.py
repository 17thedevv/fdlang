import re

with open('src/MiddleEnd/SubstitutionVisitor.cpp', 'r') as f:
    content = f.read()

# Fix CastExpr
pat_cast = r"(void SubstitutionVisitor::visit\(CastExpr& n\) \{.*?n\.)selfType( = substituteType\(std::move\(n\.)selfType(\)\);\s*\})"
content = re.sub(pat_cast, r"\1targetType\2targetType\3", content, flags=re.DOTALL)

# Fix SizeofExpr
pat_sizeof = r"(void SubstitutionVisitor::visit\(SizeofExpr& n\) \{.*?n\.)selfType( = substituteType\(std::move\(n\.)selfType(\)\);\s*\})"
content = re.sub(pat_sizeof, r"\1targetType\2targetType\3", content, flags=re.DOTALL)

# Fix AlignofExpr
pat_alignof = r"(void SubstitutionVisitor::visit\(AlignofExpr& n\) \{.*?n\.)selfType( = substituteType\(std::move\(n\.)selfType(\)\);\s*\})"
content = re.sub(pat_alignof, r"\1targetType\2targetType\3", content, flags=re.DOTALL)

with open('src/MiddleEnd/SubstitutionVisitor.cpp', 'w') as f:
    f.write(content)
print("Reverted targetType for Cast, Sizeof, Alignof")

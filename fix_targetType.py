import sys

with open('src/MiddleEnd/SubstitutionVisitor.cpp', 'r') as f:
    content = f.read()

content = content.replace("n.targetType = substituteType(std::move(n.targetType));", "n.selfType = substituteType(std::move(n.selfType));")

with open('src/MiddleEnd/SubstitutionVisitor.cpp', 'w') as f:
    f.write(content)
print("Fixed SubstitutionVisitor.cpp")

import re
with open('src/MiddleEnd/MonomorphizationEngine.cpp', 'r') as f:
    content = f.read()

content = re.sub(
    r"(\s*)std::string mangledName = Mangle::mangleStruct\(genericTemplate->name, genericArgs, symTable\);",
    r"\1std::string mangledName = Mangle::mangleStruct(genericTemplate->name, genericArgs, symTable);\n        printf(\"[DEBUG] requestStructSpecialization: %s\\n\", mangledName.c_str()); fflush(stdout);",
    content
)

with open('src/MiddleEnd/MonomorphizationEngine.cpp', 'w') as f:
    f.write(content)
print("Patched MonomorphizationEngine with printf")

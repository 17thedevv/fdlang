import re

with open('src/MiddleEnd/TypeChecker.cpp', 'r') as f:
    content = f.read()

content = re.sub(
    r"typeTable_\.resize\(table_\.symbolCount\(\), ctx_\.getUnknown\(\)\);",
    r'typeTable_.resize(table_.symbolCount(), ctx_.getUnknown());\n    printf("[DEBUG] Resized typeTable\\n"); fflush(stdout);',
    content
)

content = re.sub(
    r"root->accept\(pre\);",
    r'printf("[DEBUG] About to accept pre\\n"); fflush(stdout);\n    root->accept(pre);\n    printf("[DEBUG] Accepted pre\\n"); fflush(stdout);',
    content
)

with open('src/MiddleEnd/TypeChecker.cpp', 'w') as f:
    f.write(content)
print("Added more prints")

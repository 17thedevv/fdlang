import sys
with open('src/MiddleEnd/TypeChecker.cpp', 'r') as f:
    content = f.read()
if "const Type* selfType = ctx.unificationTable.deepResolve(typeTable[node.selfType->symbolId], ctx);" in content:
    print("Code is there.")

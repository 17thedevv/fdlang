import re

with open('src/MiddleEnd/TypeChecker.cpp', 'r') as f:
    content = f.read()

content = content.replace("args.clear();", "args.clear(); node.genericArgs.clear();")

with open('src/MiddleEnd/TypeChecker.cpp', 'w') as f:
    f.write(content)
print("Fixed infinite recursion by clearing node.genericArgs")

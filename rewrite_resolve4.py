
import re

with open("src/MiddleEnd/Resolver.cpp", "r") as f:
    content = f.read()

content = content.replace("SymbolID target = resolvePath(fullPath, tree.loc);", "SymbolID target = sm.resolvePath(fullPath, tree.loc);")

with open("src/MiddleEnd/Resolver.cpp", "w") as f:
    f.write(content)

print("Rewritten Resolver with sm.resolvePath!")

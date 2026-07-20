
import re

with open("src/MiddleEnd/Resolver.cpp", "r", encoding="utf-8") as f:
    content = f.read()

replacement = """        if (declNode) {
            if (auto* d = dynamic_cast<DeclNode*>(declNode)) {
                sym.isExported = d->isExported;
            } else if (auto* f = dynamic_cast<StructFieldNode*>(declNode)) {
                sym.isExported = f->isPublic;
            }
        }"""

content = content.replace("""        if (declNode) {
            if (auto* d = dynamic_cast<DeclNode*>(declNode)) {
                sym.isExported = d->isExported;
            }
        }""", replacement)

with open("src/MiddleEnd/Resolver.cpp", "w", encoding="utf-8") as f:
    f.write(content)

print("Patched Resolver.cpp")


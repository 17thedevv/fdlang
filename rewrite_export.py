
import re

with open("src/MiddleEnd/Resolver.cpp", "r") as f:
    content = f.read()

# Replace resolvePath export check
start_idx = content.find("if (!nextSym.isExported) {")
end_idx = content.find("}", start_idx) + 1

new_code = """if (sym.kind == SymbolKind::Module && !nextSym.isExported) {
                diag.error(loc, "Symbol '" + std::string(path[i]) + "' is private.");
                return kInvalidSymbolID;
            }"""

content = content[:start_idx] + new_code + content[end_idx:]

with open("src/MiddleEnd/Resolver.cpp", "w") as f:
    f.write(content)

print("Rewritten export check!")

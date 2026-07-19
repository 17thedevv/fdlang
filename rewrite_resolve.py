
import re

with open("src/MiddleEnd/Resolver.cpp", "r") as f:
    content = f.read()

# Replace resolvePath module restriction
start_idx = content.find("if (sym.kind != SymbolKind::Module) {")
end_idx = content.find("}", start_idx) + 1

new_code = """if (sym.kind != SymbolKind::Module && sym.kind != SymbolKind::Enum) {
                diag.error(loc, "Symbol '" + std::string(path[i-1]) + "' is not a module or enum.");
                return kInvalidSymbolID;
            }
            ScopeID nextScope = kInvalidScopeID;
            if (sym.kind == SymbolKind::Module) {
                nextScope = static_cast<ModDeclNode*>(sym.decl)->bodyScopeId;
            } else if (sym.kind == SymbolKind::Enum) {
                nextScope = static_cast<EnumDeclNode*>(sym.decl)->bodyScopeId;
            }
            auto optNext = table.lookupInScope(Identifier(path[i]), nextScope);"""

# we need to replace more than just the `if`, we need to replace up to `auto optNext...`
start_idx = content.find("if (sym.kind != SymbolKind::Module) {")
end_idx = content.find("auto optNext", start_idx)

content = content[:start_idx] + new_code + content[end_idx + len("auto optNext"):]

with open("src/MiddleEnd/Resolver.cpp", "w") as f:
    f.write(content)

print("Rewritten resolvePath!")

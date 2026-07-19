
import re

with open("src/MiddleEnd/Resolver.cpp", "r") as f:
    content = f.read()

# 1. Fix resolvePath to allow Enum
start_idx = content.find("if (sym.kind != SymbolKind::Module) {")
end_idx = content.find("auto optNext = table.lookupInScope(Identifier(path[i]), modScope);", start_idx)

new_code1 = """if (sym.kind != SymbolKind::Module && sym.kind != SymbolKind::Enum) {
                diag.error(loc, "Symbol '" + std::string(path[i-1]) + "' is not a module or enum.");
                return kInvalidSymbolID;
            }
            ScopeID nextScope = kInvalidScopeID;
            if (sym.kind == SymbolKind::Module) {
                nextScope = static_cast<ModDeclNode*>(sym.decl)->bodyScopeId;
            } else if (sym.kind == SymbolKind::Enum) {
                nextScope = static_cast<EnumDeclNode*>(sym.decl)->bodyScopeId;
            }
            """

content = content[:start_idx] + new_code1 + content[end_idx:]

# 2. Fix lookupInScope usage
content = content.replace("auto optNext = table.lookupInScope(Identifier(path[i]), modScope);", "auto optNext = table.lookupInScope(Identifier(path[i]), nextScope);")

# 3. Fix export check
start_idx2 = content.find("if (!nextSym.isExported) {")
new_code2 = "if (sym.kind == SymbolKind::Module && !nextSym.isExported) {"
content = content[:start_idx2] + new_code2 + content[start_idx2 + len("if (!nextSym.isExported) {"):]

# 4. Fix EnumPatternNode
def replace_enum_pat(s):
    start_pat = s.find("void visit(EnumPatternNode& node) override {")
    if start_pat == -1: return s
    
    # find the matching closing brace
    brace_count = 0
    end_pat = start_pat + len("void visit(EnumPatternNode& node) override {")
    brace_count = 1
    while brace_count > 0 and end_pat < len(s):
        if s[end_pat] == "{": brace_count += 1
        elif s[end_pat] == "}": brace_count -= 1
        end_pat += 1
    
    new_pat = """void visit(EnumPatternNode& node) override { 
        if (!node.path.empty()) {
            SymbolID varId = sm.resolvePath(node.path, node.loc);
            if (varId != kInvalidSymbolID) {
                auto sym = sm.table.getSymbol(varId);
                if (sym.kind == SymbolKind::EnumVariant) {
                    node.variantSymbolId = varId;
                } else {
                    diag.error(node.loc, "'" + std::string(node.path.back()) + "' is not an enum variant");
                }
            }
        }
        for (auto& p : node.fields) { p->accept(static_cast<PatternVisitor&>(*this)); }
    }"""
    
    return s[:start_pat] + new_pat + s[end_pat:]

content = replace_enum_pat(content)
content = replace_enum_pat(content) # Do it twice for both visitors

with open("src/MiddleEnd/Resolver.cpp", "w") as f:
    f.write(content)

print("Rewritten Resolver!")


import re

with open("src/MiddleEnd/Resolver.cpp", "r") as f:
    content = f.read()

# Remove resolvePath from UseResolutionVisitor
start_idx = content.find("SymbolID resolvePath(const std::vector<std::string_view>& path, SourceLocation loc) {")
end_idx = content.find("void processUseTree", start_idx)

# Find the matching brace for resolvePath to be precise, or just use end_idx since we know what it looks like.
code_to_remove = content[start_idx:end_idx]
content = content[:start_idx] + content[end_idx:]

# Add resolvePath to ScopeManager
sm_end = content.find("SymbolTable& table;")
resolvePathCode = """
    SymbolID resolvePath(const std::vector<std::string_view>& path, SourceLocation loc) {
        if (path.empty()) return kInvalidSymbolID;
        SymbolID currentSym = resolve(path[0], loc);
        if (currentSym == kInvalidSymbolID) return currentSym;

        for (size_t i = 1; i < path.size(); ++i) {
            auto& sym = table.getSymbol(currentSym);
            if (sym.kind != SymbolKind::Module && sym.kind != SymbolKind::Enum) {
                diag.error(loc, "Symbol '" + std::string(path[i-1]) + "' is not a module or enum.");
                return kInvalidSymbolID;
            }
            ScopeID nextScope = kInvalidScopeID;
            if (sym.kind == SymbolKind::Module) {
                nextScope = static_cast<ModDeclNode*>(sym.decl)->bodyScopeId;
            } else if (sym.kind == SymbolKind::Enum) {
                nextScope = static_cast<EnumDeclNode*>(sym.decl)->bodyScopeId;
            }
            auto optNext = table.lookupInScope(Identifier(path[i]), nextScope);
            if (!optNext) {
                diag.error(loc, "Module/Enum '" + std::string(path[i-1]) + "' does not contain '" + std::string(path[i]) + "'.");
                return kInvalidSymbolID;
            }
            auto& nextSym = table.getSymbol(*optNext);
            if (sym.kind == SymbolKind::Module && !nextSym.isExported) {
                diag.error(loc, "Symbol '" + std::string(path[i]) + "' is private.");
                return kInvalidSymbolID;
            }
            currentSym = *optNext;
        }
        return currentSym;
    }

    """
content = content[:sm_end] + resolvePathCode + content[sm_end:]

with open("src/MiddleEnd/Resolver.cpp", "w") as f:
    f.write(content)

print("Rewritten Resolver with resolvePath in ScopeManager!")

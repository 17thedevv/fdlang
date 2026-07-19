
import re

with open("src/MiddleEnd/Resolver.cpp", "r") as f:
    content = f.read()

start_idx = content.find("void visit(EnumPatternNode& node) override {")
end_idx = content.find("}", start_idx) + 1
while content[end_idx] != "}":
    end_idx = content.find("}", end_idx) + 1

new_code = """void visit(EnumPatternNode& node) override { 
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

content = content[:start_idx] + new_code + content[end_idx:]

with open("src/MiddleEnd/Resolver.cpp", "w") as f:
    f.write(content)

print("Rewritten EnumPatternNode!")

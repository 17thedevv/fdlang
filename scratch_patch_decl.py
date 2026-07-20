
import re

with open("include/mellis/AST/DeclNode.h", "r", encoding="utf-8") as f:
    content = f.read()

content = content.replace("""class StructFieldNode : public ASTNode {
public:
    std::string_view          name;
    std::unique_ptr<TypeNode> type;
    SymbolID                  symbolId = kInvalidSymbolID;
    void accept(ASTVisitor& v) override;""", """class StructFieldNode : public ASTNode {
public:
    std::string_view          name;
    std::unique_ptr<TypeNode> type;
    SymbolID                  symbolId = kInvalidSymbolID;
    bool                      isPublic = false;
    void accept(ASTVisitor& v) override;""")

with open("include/mellis/AST/DeclNode.h", "w", encoding="utf-8") as f:
    f.write(content)

print("Patched DeclNode.h")


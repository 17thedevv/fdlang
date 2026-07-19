import sys

with open('src/MiddleEnd/TypeChecker.cpp', 'r') as f:
    content = f.read()

target = '''          void visit(TupleLiteralExpr& node) override {}
          void visit(StructInitExpr& node) override {}
            void visit(MatchExpr& node) override {'''

replacement = '''          void visit(TupleLiteralExpr& node) override {}
          void visit(StructInitExpr& node) override {
              resolve(node.inferredType, node.loc);
              for (auto& field : node.fields) {
                  if (field.value) field.value->accept(*this);
              }
          }
            void visit(MatchExpr& node) override {'''

if target in content:
    content = content.replace(target, replacement)
    with open('src/MiddleEnd/TypeChecker.cpp', 'w') as f:
        f.write(content)
    print("Patched TypeResolver StructInitExpr successfully")
else:
    print("Target not found")

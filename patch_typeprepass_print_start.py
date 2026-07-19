import sys

with open('src/MiddleEnd/TypeChecker.cpp', 'r') as f:
    content = f.read()

target = '''        void visit(ImplDeclNode& node) override {
            if (!node.genericParams.empty()) return;
              const Type* selfType = evaluateTypeNode(node.selfType.get());'''

replacement = '''        void visit(ImplDeclNode& node) override {
            printf("[DEBUG TypePrePass] visiting ImplDeclNode\n");
            if (!node.genericParams.empty()) {
                printf("[DEBUG TypePrePass] ImplDeclNode has generic parameters\n");
                return;
            }
            const Type* selfType = evaluateTypeNode(node.selfType.get());
            if (!selfType) {
                printf("[DEBUG TypePrePass] selfType is null\n");
                return;
            }'''

if target in content:
    content = content.replace(target, replacement)
    with open('src/MiddleEnd/TypeChecker.cpp', 'w') as f:
        f.write(content)
    print("Patched TypePrePass ImplDeclNode start successfully")
else:
    print("Target not found")

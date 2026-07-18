import re

def add_clone(file_path, classes):
    with open(file_path, 'r') as f:
        content = f.read()

    for cls in classes:
        pattern = re.compile(rf'(class {cls} : public [^\{{]+\{{[\s\S]*?void accept\(ASTVisitor& v\) override;)([\s\S]*?\}};)')
        content = pattern.sub(r'\1\n    ASTNode* cloneImpl() const override;\2', content)

    with open(file_path, 'w') as f:
        f.write(content)

add_clone('d:/fdlang/include/fdlang/AST/ExprNode.h', ['IdentifierExpr', 'StructInitExpr', 'CallExpr', 'MethodCallExpr'])

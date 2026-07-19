import re
import sys

def add_clone(filepath):
    with open(filepath, 'r') as f:
        content = f.read()

    classes = re.findall(r'class\s+(\w+)\s*:\s*public\s+(?:ExprNode|StmtNode)\s*{([^}]*?)};', content)
    
    for cls_name, cls_body in classes:
        if 'cloneImpl' not in cls_body:
            # find end of class
            pattern = re.compile(r'(class\s+' + cls_name + r'\s*:\s*public\s+(?:ExprNode|StmtNode)\s*{[^}]*?)(\s*};)')
            match = pattern.search(content)
            if match:
                content = content[:match.start(2)] + '\n    ASTNode* cloneImpl() const override;' + content[match.start(2):]

    with open(filepath, 'w') as f:
        f.write(content)

add_clone('d:/fdlang/include/mellis/AST/ExprNode.h')
add_clone('d:/fdlang/include/mellis/AST/StmtNode.h')
print("Done adding cloneImpl to headers")

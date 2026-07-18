
import sys
content = open('src/MiddleEnd/TypeChecker.cpp', 'r', encoding='utf-8').read()
content = content.replace('        void visit(DeferStmtNode& node) override { node.call->accept(*this); }\n', '')
with open('src/MiddleEnd/TypeChecker.cpp', 'w', encoding='utf-8') as f:
    f.write(content)

#pragma once
#include "fdlang/AST/ASTNode.h"
#include <vector>
#include <memory>

namespace fl {

class DeclNode;

class ProgramNode : public ASTNode {
public:
    std::vector<std::unique_ptr<DeclNode>> decls;
    void accept(ASTVisitor& v) override;
};

} // namespace fl

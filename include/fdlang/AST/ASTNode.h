#pragma once
#include <memory>
#include <vector>

namespace fl {
    class ASTNode {
    public:
        virtual ~ASTNode() = default;
        virtual void accept(class ASTVisitor* visitor) = 0;
    };

    class StmtNode; // Forward declaration

    class ProgramNode : public ASTNode {
    public:
        std::vector<std::unique_ptr<StmtNode>> statements;
        void accept(ASTVisitor* visitor) override;
    };
}
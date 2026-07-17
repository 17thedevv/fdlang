#pragma once
#include <cstdint>

namespace fl {

class ASTVisitor;

struct SourceLocation {
    uint32_t byteOffset = 0;
};

class ASTNode {
public:
    SourceLocation loc;
    virtual ~ASTNode() = default;
    virtual void accept(ASTVisitor& v) = 0;
};

class ItemNode : public ASTNode {};

} // namespace fl
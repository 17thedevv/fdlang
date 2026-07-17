#pragma once
#include "fdlang/AST/ASTNode.h"
#include "fdlang/Core/Types.h"
#include <vector>
#include <memory>
#include <string_view>

namespace fl {

class LiteralExpr;
class PatternVisitor;

class PatternNode : public ASTNode {
public:
    virtual void accept(PatternVisitor& v) = 0;
};

class WildcardPatternNode : public PatternNode {
public:
    void accept(PatternVisitor& v) override;
    void accept(ASTVisitor& v) override { }
};

class LiteralPatternNode : public PatternNode {
public:
    std::unique_ptr<LiteralExpr> lit;
    void accept(PatternVisitor& v) override;
    void accept(ASTVisitor& v) override { }
};

class IdentifierPatternNode : public PatternNode {
public:
    std::vector<std::string_view> segments;
    SymbolID                      symbolId = kInvalidSymbolID;
    void accept(PatternVisitor& v) override;
    void accept(ASTVisitor& v) override { }
};

class EnumPatternNode : public PatternNode {
public:
    std::vector<std::string_view>           path;
    std::vector<std::unique_ptr<PatternNode>> fields;
    void accept(PatternVisitor& v) override;
    void accept(ASTVisitor& v) override { }
};

class TuplePatternNode : public PatternNode {
public:
    std::vector<std::unique_ptr<PatternNode>> elements;
    void accept(PatternVisitor& v) override;
    void accept(ASTVisitor& v) override { }
};

} // namespace fl

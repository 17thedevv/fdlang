#pragma once
#include "ASTNode.h"
#include "fdlang/FrontEnd/ASTVisitor.h"
#include "fdlang/Core/Types.h"          // SymbolID, kInvalidSymbolID
#include "fdlang/Core/FLType.h"         // FLType — annotated by TypeChecker
#include <string_view>
#include <memory>

namespace fl {

class ExprNode : public ASTNode {
public:
    // ── TypeChecker annotation ────────────────────────────────────────────────
    // Set to FLType::Unknown by default (before TypeChecker runs).
    // Filled in by TypeChecker pass on every expression node.
    // Read by FLIRGenerator for code-generation decisions.
    // BorrowChecker may also consult it for ownership rules on non-trivial types.
    FLType inferredType = FLType::Unknown;
};


class NumberExpr : public ExprNode {
public:
    std::string_view value;
    explicit NumberExpr(std::string_view val) : value(val) {}

    void accept(ASTVisitor* visitor) override {
        visitor->visit(this); // "Mời anh Visitor vào phòng NumberExpr"
    }
};

class IdentifierExpr : public ExprNode {
public:
    std::string_view name;

    // ── Resolver annotation ──────────────────────────────────────────────
    // Set to kInvalidSymbolID by the Parser.
    // Filled in by the Resolver pass: connects this usage to its declaration.
    // TypeChecker / BorrowChecker / IRGenerator read this to find the Symbol.
    SymbolID symbolId = kInvalidSymbolID;

    explicit IdentifierExpr(std::string_view n) : name(n) {}

    void accept(ASTVisitor* visitor) override {
        visitor->visit(this);
    }
};

    class BooleanExpr : public ExprNode {
    public:
     bool value;
     explicit BooleanExpr(bool val) : value(val) {}
     void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    };

    class BinaryExpr : public ExprNode {
    public:
        std::unique_ptr<ExprNode> left;
        std::string_view op; // Toán tử (+, -, *, /, ==, <...)
        std::unique_ptr<ExprNode> right;

     BinaryExpr(std::unique_ptr<ExprNode> left, std::string_view op, std::unique_ptr<ExprNode> right)
        : left(std::move(left)), op(op), right(std::move(right)) {}

     void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    };
} // namespace fl
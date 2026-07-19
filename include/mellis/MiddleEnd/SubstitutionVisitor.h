#pragma once
#include "mellis/FrontEnd/ASTVisitor.h"
#include "mellis/AST/TypeNode.h"
#include <unordered_map>
#include <string>
#include <string_view>
#include <memory>

namespace fl {

class SubstitutionVisitor : public ASTVisitor {
public:
    std::unordered_map<std::string, std::unique_ptr<TypeNode>> substitutions;

    SubstitutionVisitor(std::unordered_map<std::string, std::unique_ptr<TypeNode>> subs)
        : substitutions(std::move(subs)) {}

    void substitute(ASTNode& node) {
        node.accept(*this);
    }

    std::unique_ptr<TypeNode> substituteType(std::unique_ptr<TypeNode> type);

    // Decl
    void visit(ProgramNode&) override {}
    void visit(VarDeclNode&) override;
    void visit(FunctionDeclNode&) override;
    void visit(ParamDeclNode&) override;
    void visit(StructDeclNode&) override;
    void visit(StructFieldNode&) override;
    void visit(EnumDeclNode&) override {}
    void visit(EnumVariantNode&) override {}
    void visit(TraitDeclNode&) override {}
    void visit(ImplDeclNode&) override;
    void visit(ModDeclNode&) override {}
    void visit(UseDeclNode&) override {}
    void visit(ExternDeclNode&) override {}
    void visit(TypeAliasDeclNode&) override {}

    // Stmt
    void visit(BlockStmtNode&) override;
    void visit(ExprStmtNode&) override;
    void visit(IfStmtNode&) override;
    void visit(WhileStmtNode&) override;
    void visit(ForStmtNode&) override;
    void visit(ReturnStmtNode&) override;
    void visit(BreakStmtNode&) override {}
    void visit(ContinueStmtNode&) override {}
    void visit(UnsafeStmtNode&) override;
    void visit(ComptimeStmtNode&) override;

    // Expr
    void visit(LiteralExpr&) override {}
    void visit(IdentifierExpr&) override;
    void visit(BinaryExpr&) override;
    void visit(UnaryExpr&) override;
    void visit(AssignExpr&) override;
    void visit(CallExpr&) override;
    void visit(MethodCallExpr&) override;
    void visit(IndexExpr&) override;
    void visit(MemberExpr&) override;
    void visit(TupleIndexExpr&) override;
    void visit(CastExpr&) override;
    void visit(UnsizeCastExpr&) override;
    void visit(ArrayLiteralExpr&) override;
    void visit(TupleLiteralExpr&) override;
    void visit(StructInitExpr&) override;
    void visit(MatchExpr&) override;
    void visit(LambdaExpr&) override;
    void visit(AwaitExpr&) override;
    void visit(SizeofExpr&) override;
    void visit(AlignofExpr&) override;
};

} // namespace fl

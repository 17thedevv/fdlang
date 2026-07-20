#pragma once
#include "mellis/AST/ASTNode.h"
#include "mellis/AST/DeclNode.h"
#include "mellis/AST/ExprNode.h"
#include "mellis/AST/StmtNode.h"
#include "mellis/AST/TypeNode.h"
#include "mellis/AST/PatternNode.h"
#include "mellis/AST/MacroNode.h"
#include <memory>

namespace fl {

class ASTTransformer {
public:
    virtual ~ASTTransformer() = default;

    std::unique_ptr<DeclNode> transformDecl(std::unique_ptr<DeclNode> node);
    std::unique_ptr<ExprNode> transformExpr(std::unique_ptr<ExprNode> node);
    std::unique_ptr<StmtNode> transformStmt(std::unique_ptr<StmtNode> node);
    std::unique_ptr<TypeNode> transformType(std::unique_ptr<TypeNode> node);
    std::unique_ptr<PatternNode> transformPattern(std::unique_ptr<PatternNode> node);

    virtual std::unique_ptr<DeclNode> transform(std::unique_ptr<VarDeclNode> node);
    virtual std::unique_ptr<DeclNode> transform(std::unique_ptr<ParamDeclNode> node);
    virtual std::unique_ptr<DeclNode> transform(std::unique_ptr<FunctionDeclNode> node);
    virtual std::unique_ptr<ASTNode> transform(std::unique_ptr<StructFieldNode> node);
    virtual std::unique_ptr<DeclNode> transform(std::unique_ptr<StructDeclNode> node);
    virtual std::unique_ptr<StmtNode> transform(std::unique_ptr<BlockStmtNode> node);
    virtual std::unique_ptr<StmtNode> transform(std::unique_ptr<ExprStmtNode> node);
    virtual std::unique_ptr<StmtNode> transform(std::unique_ptr<ReturnStmtNode> node);
    virtual std::unique_ptr<ExprNode> transform(std::unique_ptr<IdentifierExpr> node);
    virtual std::unique_ptr<ExprNode> transform(std::unique_ptr<StructInitExpr> node);
    virtual std::unique_ptr<ExprNode> transform(std::unique_ptr<CallExpr> node);
    virtual std::unique_ptr<ExprNode> transform(std::unique_ptr<MethodCallExpr> node);
    virtual std::unique_ptr<TypeNode> transform(std::unique_ptr<BuiltinTypeNode> node);
    virtual std::unique_ptr<TypeNode> transform(std::unique_ptr<NeverTypeNode> node);
    virtual std::unique_ptr<TypeNode> transform(std::unique_ptr<TraitObjectTypeNode> node);
    virtual std::unique_ptr<TypeNode> transform(std::unique_ptr<NamedTypeNode> node);
    virtual std::unique_ptr<TypeNode> transform(std::unique_ptr<ReferenceTypeNode> node);
    virtual std::unique_ptr<TypeNode> transform(std::unique_ptr<PointerTypeNode> node);
    virtual std::unique_ptr<TypeNode> transform(std::unique_ptr<ArrayTypeNode> node);
    virtual std::unique_ptr<TypeNode> transform(std::unique_ptr<TupleTypeNode> node);
    virtual std::unique_ptr<TypeNode> transform(std::unique_ptr<FunctionTypeNode> node);
    virtual std::unique_ptr<DeclNode> transform(std::unique_ptr<TraitDeclNode> node);
    virtual std::unique_ptr<DeclNode> transform(std::unique_ptr<ImplDeclNode> node);
    virtual std::unique_ptr<ExprNode> transform(std::unique_ptr<LiteralExpr> node);
    virtual std::unique_ptr<ExprNode> transform(std::unique_ptr<BinaryExpr> node);
    virtual std::unique_ptr<ExprNode> transform(std::unique_ptr<UnaryExpr> node);
    virtual std::unique_ptr<ExprNode> transform(std::unique_ptr<AssignExpr> node);
    virtual std::unique_ptr<ExprNode> transform(std::unique_ptr<IndexExpr> node);
    virtual std::unique_ptr<ExprNode> transform(std::unique_ptr<MemberExpr> node);
    virtual std::unique_ptr<ExprNode> transform(std::unique_ptr<CastExpr> node);
    virtual std::unique_ptr<ExprNode> transform(std::unique_ptr<ArrayLiteralExpr> node);
    virtual std::unique_ptr<ExprNode> transform(std::unique_ptr<TupleLiteralExpr> node);
    virtual std::unique_ptr<ExprNode> transform(std::unique_ptr<MatchExpr> node);
    virtual std::unique_ptr<ExprNode> transform(std::unique_ptr<LambdaExpr> node);
    virtual std::unique_ptr<ExprNode> transform(std::unique_ptr<AwaitExpr> node);
    virtual std::unique_ptr<ExprNode> transform(std::unique_ptr<SizeofExpr> node);
    virtual std::unique_ptr<ExprNode> transform(std::unique_ptr<AlignofExpr> node);
    virtual std::unique_ptr<StmtNode> transform(std::unique_ptr<IfStmtNode> node);
    virtual std::unique_ptr<StmtNode> transform(std::unique_ptr<WhileStmtNode> node);
    virtual std::unique_ptr<StmtNode> transform(std::unique_ptr<ForStmtNode> node);
    virtual std::unique_ptr<StmtNode> transform(std::unique_ptr<BreakStmtNode> node);
    virtual std::unique_ptr<StmtNode> transform(std::unique_ptr<ContinueStmtNode> node);
    virtual std::unique_ptr<StmtNode> transform(std::unique_ptr<UnsafeStmtNode> node);
    virtual std::unique_ptr<StmtNode> transform(std::unique_ptr<ComptimeStmtNode> node);
    virtual std::unique_ptr<ExprNode> transform(std::unique_ptr<PlaceholderExpr> node);
    virtual std::unique_ptr<StmtNode> transform(std::unique_ptr<PlaceholderStmt> node);
    virtual std::unique_ptr<TypeNode> transform(std::unique_ptr<PlaceholderTypeNode> node);
    virtual std::unique_ptr<DeclNode> transform(std::unique_ptr<MacroDeclNode> node);
    virtual std::unique_ptr<ExprNode> transform(std::unique_ptr<MacroCallExpr> node);
    virtual std::unique_ptr<StmtNode> transform(std::unique_ptr<MacroCallStmt> node);
    virtual std::unique_ptr<StmtNode> transform(std::unique_ptr<MacroExpandForStmt> node);
};
} // namespace fl

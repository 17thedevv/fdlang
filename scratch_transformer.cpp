#include "mellis/FrontEnd/ASTTransformer.h"

namespace fl {
std::unique_ptr<DeclNode> ASTTransformer::transform(std::unique_ptr<VarDeclNode> node) {
    if (node->typeAnnot) node->typeAnnot = transformType(std::move(node->typeAnnot));
    return node;
}

std::unique_ptr<DeclNode> ASTTransformer::transform(std::unique_ptr<ParamDeclNode> node) {
    if (node->type) node->type = transformType(std::move(node->type));
    return node;
}

std::unique_ptr<DeclNode> ASTTransformer::transform(std::unique_ptr<FunctionDeclNode> node) {
    for (auto& item : node->genericParams) { item = transform(std::move(item)); }
    return node;
}

std::unique_ptr<ASTNode> ASTTransformer::transform(std::unique_ptr<StructFieldNode> node) {
    if (node->type) node->type = transformType(std::move(node->type));
    return node;
}

std::unique_ptr<DeclNode> ASTTransformer::transform(std::unique_ptr<StructDeclNode> node) {
    for (auto& item : node->genericParams) { item = transform(std::move(item)); }
    return node;
}

std::unique_ptr<StmtNode> ASTTransformer::transform(std::unique_ptr<BlockStmtNode> node) {
    for (auto& item : node->body) { item = transform(std::move(item)); }
    return node;
}

std::unique_ptr<StmtNode> ASTTransformer::transform(std::unique_ptr<ExprStmtNode> node) {
    if (node->expr) node->expr = transformExpr(std::move(node->expr));
    return node;
}

std::unique_ptr<StmtNode> ASTTransformer::transform(std::unique_ptr<ReturnStmtNode> node) {
    if (node->value) node->value = transformExpr(std::move(node->value));
    return node;
}

std::unique_ptr<ExprNode> ASTTransformer::transform(std::unique_ptr<IdentifierExpr> node) {
    for (auto& item : node->genericArgs) { item = transform(std::move(item)); }
    return node;
}

std::unique_ptr<ExprNode> ASTTransformer::transform(std::unique_ptr<StructInitExpr> node) {
    for (auto& item : node->genericArgs) { item = transform(std::move(item)); }
    return node;
}

std::unique_ptr<ExprNode> ASTTransformer::transform(std::unique_ptr<CallExpr> node) {
    if (node->callee) node->callee = transformExpr(std::move(node->callee));
    return node;
}

std::unique_ptr<ExprNode> ASTTransformer::transform(std::unique_ptr<MethodCallExpr> node) {
    if (node->object) node->object = transformExpr(std::move(node->object));
    return node;
}

std::unique_ptr<TypeNode> ASTTransformer::transform(std::unique_ptr<BuiltinTypeNode> node) {
    return node;
}

std::unique_ptr<TypeNode> ASTTransformer::transform(std::unique_ptr<NeverTypeNode> node) {
    return node;
}

std::unique_ptr<TypeNode> ASTTransformer::transform(std::unique_ptr<TraitObjectTypeNode> node) {
    if (node->trait) node->trait = transformType(std::move(node->trait));
    return node;
}

std::unique_ptr<TypeNode> ASTTransformer::transform(std::unique_ptr<NamedTypeNode> node) {
    for (auto& item : node->genericArgs) { item = transform(std::move(item)); }
    return node;
}

std::unique_ptr<TypeNode> ASTTransformer::transform(std::unique_ptr<ReferenceTypeNode> node) {
    if (node->inner) node->inner = transformType(std::move(node->inner));
    return node;
}

std::unique_ptr<TypeNode> ASTTransformer::transform(std::unique_ptr<PointerTypeNode> node) {
    if (node->inner) node->inner = transformType(std::move(node->inner));
    return node;
}

std::unique_ptr<TypeNode> ASTTransformer::transform(std::unique_ptr<ArrayTypeNode> node) {
    if (node->elementType) node->elementType = transformType(std::move(node->elementType));
    return node;
}

std::unique_ptr<TypeNode> ASTTransformer::transform(std::unique_ptr<TupleTypeNode> node) {
    for (auto& item : node->elements) { item = transform(std::move(item)); }
    return node;
}

std::unique_ptr<TypeNode> ASTTransformer::transform(std::unique_ptr<FunctionTypeNode> node) {
    for (auto& item : node->params) { item = transform(std::move(item)); }
    return node;
}

std::unique_ptr<DeclNode> ASTTransformer::transform(std::unique_ptr<TraitDeclNode> node) {
    for (auto& item : node->genericParams) { item = transform(std::move(item)); }
    return node;
}

std::unique_ptr<DeclNode> ASTTransformer::transform(std::unique_ptr<ImplDeclNode> node) {
    for (auto& item : node->genericParams) { item = transform(std::move(item)); }
    return node;
}

std::unique_ptr<ExprNode> ASTTransformer::transform(std::unique_ptr<LiteralExpr> node) {
    return node;
}

std::unique_ptr<ExprNode> ASTTransformer::transform(std::unique_ptr<BinaryExpr> node) {
    if (node->left) node->left = transformExpr(std::move(node->left));
    if (node->right) node->right = transformExpr(std::move(node->right));
    return node;
}

std::unique_ptr<ExprNode> ASTTransformer::transform(std::unique_ptr<UnaryExpr> node) {
    if (node->operand) node->operand = transformExpr(std::move(node->operand));
    return node;
}

std::unique_ptr<ExprNode> ASTTransformer::transform(std::unique_ptr<AssignExpr> node) {
    if (node->lvalue) node->lvalue = transformExpr(std::move(node->lvalue));
    if (node->value) node->value = transformExpr(std::move(node->value));
    return node;
}

std::unique_ptr<ExprNode> ASTTransformer::transform(std::unique_ptr<IndexExpr> node) {
    if (node->base) node->base = transformExpr(std::move(node->base));
    if (node->index) node->index = transformExpr(std::move(node->index));
    return node;
}

std::unique_ptr<ExprNode> ASTTransformer::transform(std::unique_ptr<MemberExpr> node) {
    if (node->object) node->object = transformExpr(std::move(node->object));
    return node;
}

std::unique_ptr<ExprNode> ASTTransformer::transform(std::unique_ptr<CastExpr> node) {
    if (node->expr) node->expr = transformExpr(std::move(node->expr));
    if (node->targetType) node->targetType = transformType(std::move(node->targetType));
    return node;
}

std::unique_ptr<ExprNode> ASTTransformer::transform(std::unique_ptr<ArrayLiteralExpr> node) {
    for (auto& item : node->elements) { item = transform(std::move(item)); }
    return node;
}

std::unique_ptr<ExprNode> ASTTransformer::transform(std::unique_ptr<TupleLiteralExpr> node) {
    for (auto& item : node->elements) { item = transform(std::move(item)); }
    return node;
}

std::unique_ptr<ExprNode> ASTTransformer::transform(std::unique_ptr<MatchExpr> node) {
    if (node->subject) node->subject = transformExpr(std::move(node->subject));
    return node;
}

std::unique_ptr<ExprNode> ASTTransformer::transform(std::unique_ptr<LambdaExpr> node) {
    return node;
}

std::unique_ptr<ExprNode> ASTTransformer::transform(std::unique_ptr<AwaitExpr> node) {
    if (node->expr) node->expr = transformExpr(std::move(node->expr));
    return node;
}

std::unique_ptr<ExprNode> ASTTransformer::transform(std::unique_ptr<SizeofExpr> node) {
    if (node->targetType) node->targetType = transformType(std::move(node->targetType));
    return node;
}

std::unique_ptr<ExprNode> ASTTransformer::transform(std::unique_ptr<AlignofExpr> node) {
    if (node->targetType) node->targetType = transformType(std::move(node->targetType));
    return node;
}

std::unique_ptr<StmtNode> ASTTransformer::transform(std::unique_ptr<IfStmtNode> node) {
    if (node->condition) node->condition = transformExpr(std::move(node->condition));
    if (node->thenBranch) node->thenBranch = transform(std::move(node->thenBranch));
    if (node->elseBranch) node->elseBranch = transformStmt(std::move(node->elseBranch));
    return node;
}

std::unique_ptr<StmtNode> ASTTransformer::transform(std::unique_ptr<WhileStmtNode> node) {
    if (node->condition) node->condition = transformExpr(std::move(node->condition));
    if (node->body) node->body = transform(std::move(node->body));
    return node;
}

std::unique_ptr<StmtNode> ASTTransformer::transform(std::unique_ptr<ForStmtNode> node) {
    if (node->iterable) node->iterable = transformExpr(std::move(node->iterable));
    if (node->init) node->init = transform(std::move(node->init));
    if (node->cond) node->cond = transformExpr(std::move(node->cond));
    if (node->step) node->step = transformExpr(std::move(node->step));
    if (node->body) node->body = transform(std::move(node->body));
    return node;
}

std::unique_ptr<StmtNode> ASTTransformer::transform(std::unique_ptr<BreakStmtNode> node) {
    return node;
}

std::unique_ptr<StmtNode> ASTTransformer::transform(std::unique_ptr<ContinueStmtNode> node) {
    return node;
}

std::unique_ptr<StmtNode> ASTTransformer::transform(std::unique_ptr<UnsafeStmtNode> node) {
    if (node->body) node->body = transform(std::move(node->body));
    return node;
}

std::unique_ptr<StmtNode> ASTTransformer::transform(std::unique_ptr<ComptimeStmtNode> node) {
    if (node->body) node->body = transform(std::move(node->body));
    return node;
}

std::unique_ptr<ExprNode> ASTTransformer::transform(std::unique_ptr<PlaceholderExpr> node) {
    return node;
}

std::unique_ptr<StmtNode> ASTTransformer::transform(std::unique_ptr<PlaceholderStmt> node) {
    return node;
}

std::unique_ptr<TypeNode> ASTTransformer::transform(std::unique_ptr<PlaceholderTypeNode> node) {
    return node;
}

std::unique_ptr<DeclNode> ASTTransformer::transform(std::unique_ptr<MacroDeclNode> node) {
    if (node->body) node->body = transform(std::move(node->body));
    return node;
}

std::unique_ptr<ExprNode> ASTTransformer::transform(std::unique_ptr<MacroCallExpr> node) {
    for (auto& item : node->args) { item = transform(std::move(item)); }
    return node;
}

std::unique_ptr<StmtNode> ASTTransformer::transform(std::unique_ptr<MacroCallStmt> node) {
    for (auto& item : node->args) { item = transform(std::move(item)); }
    return node;
}

std::unique_ptr<StmtNode> ASTTransformer::transform(std::unique_ptr<MacroExpandForStmt> node) {
    if (node->body) node->body = transform(std::move(node->body));
    return node;
}


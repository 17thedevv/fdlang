import sys

def generate_clone_impls():
    out = "\n// --- GENERATED CLONES ---\n"
    
    # LiteralExpr
    out += """
ASTNode* LiteralExpr::cloneImpl() const {
    auto copy = new LiteralExpr();
    copy->loc = this->loc;
    copy->inferredType = this->inferredType;
    copy->valueCategory = this->valueCategory;
    copy->isConstant = this->isConstant;
    copy->kind = this->kind;
    copy->suffix = this->suffix;
    copy->rawText = this->rawText;
    copy->value = this->value;
    return copy;
}
"""
    # BinaryExpr
    out += """
ASTNode* BinaryExpr::cloneImpl() const {
    auto copy = new BinaryExpr();
    copy->loc = this->loc;
    copy->inferredType = this->inferredType;
    copy->valueCategory = this->valueCategory;
    copy->isConstant = this->isConstant;
    copy->op = this->op;
    if (this->left) copy->left = this->left->cloneAs<ExprNode>();
    if (this->right) copy->right = this->right->cloneAs<ExprNode>();
    return copy;
}
"""
    # UnaryExpr
    out += """
ASTNode* UnaryExpr::cloneImpl() const {
    auto copy = new UnaryExpr();
    copy->loc = this->loc;
    copy->inferredType = this->inferredType;
    copy->valueCategory = this->valueCategory;
    copy->isConstant = this->isConstant;
    copy->op = this->op;
    if (this->operand) copy->operand = this->operand->cloneAs<ExprNode>();
    return copy;
}
"""
    # AssignExpr
    out += """
ASTNode* AssignExpr::cloneImpl() const {
    auto copy = new AssignExpr();
    copy->loc = this->loc;
    copy->inferredType = this->inferredType;
    copy->valueCategory = this->valueCategory;
    copy->isConstant = this->isConstant;
    copy->op = this->op;
    if (this->lvalue) copy->lvalue = this->lvalue->cloneAs<ExprNode>();
    if (this->value) copy->value = this->value->cloneAs<ExprNode>();
    return copy;
}
"""
    # IndexExpr
    out += """
ASTNode* IndexExpr::cloneImpl() const {
    auto copy = new IndexExpr();
    copy->loc = this->loc;
    copy->inferredType = this->inferredType;
    copy->valueCategory = this->valueCategory;
    copy->isConstant = this->isConstant;
    if (this->base) copy->base = this->base->cloneAs<ExprNode>();
    if (this->index) copy->index = this->index->cloneAs<ExprNode>();
    return copy;
}
"""
    # MemberExpr
    out += """
ASTNode* MemberExpr::cloneImpl() const {
    auto copy = new MemberExpr();
    copy->loc = this->loc;
    copy->inferredType = this->inferredType;
    copy->valueCategory = this->valueCategory;
    copy->isConstant = this->isConstant;
    if (this->object) copy->object = this->object->cloneAs<ExprNode>();
    copy->member = this->member;
    copy->memberId = this->memberId;
    copy->resolvedFieldIndex = this->resolvedFieldIndex;
    return copy;
}
"""
    # CastExpr
    out += """
ASTNode* CastExpr::cloneImpl() const {
    auto copy = new CastExpr();
    copy->loc = this->loc;
    copy->inferredType = this->inferredType;
    copy->valueCategory = this->valueCategory;
    copy->isConstant = this->isConstant;
    if (this->expr) copy->expr = this->expr->cloneAs<ExprNode>();
    if (this->targetType) copy->targetType = this->targetType->cloneAs<TypeNode>();
    return copy;
}
"""
    # ArrayLiteralExpr
    out += """
ASTNode* ArrayLiteralExpr::cloneImpl() const {
    auto copy = new ArrayLiteralExpr();
    copy->loc = this->loc;
    copy->inferredType = this->inferredType;
    copy->valueCategory = this->valueCategory;
    copy->isConstant = this->isConstant;
    for (const auto& e : this->elements) {
        copy->elements.push_back(e->cloneAs<ExprNode>());
    }
    return copy;
}
"""
    # TupleLiteralExpr
    out += """
ASTNode* TupleLiteralExpr::cloneImpl() const {
    auto copy = new TupleLiteralExpr();
    copy->loc = this->loc;
    copy->inferredType = this->inferredType;
    copy->valueCategory = this->valueCategory;
    copy->isConstant = this->isConstant;
    for (const auto& e : this->elements) {
        copy->elements.push_back(e->cloneAs<ExprNode>());
    }
    return copy;
}
"""
    # MatchExpr
    out += """
ASTNode* MatchExpr::cloneImpl() const {
    auto copy = new MatchExpr();
    copy->loc = this->loc;
    copy->inferredType = this->inferredType;
    copy->valueCategory = this->valueCategory;
    copy->isConstant = this->isConstant;
    if (this->subject) copy->subject = this->subject->cloneAs<ExprNode>();
    return copy;
}
"""
    # LambdaExpr
    out += """
ASTNode* LambdaExpr::cloneImpl() const {
    auto copy = new LambdaExpr();
    copy->loc = this->loc;
    copy->inferredType = this->inferredType;
    copy->valueCategory = this->valueCategory;
    copy->isConstant = this->isConstant;
    return copy;
}
"""
    # AwaitExpr, SizeofExpr, AlignofExpr
    out += """
ASTNode* AwaitExpr::cloneImpl() const { 
    auto copy = new AwaitExpr();
    copy->loc = this->loc;
    if (this->expr) copy->expr = this->expr->cloneAs<ExprNode>();
    return copy;
}
ASTNode* SizeofExpr::cloneImpl() const { 
    auto copy = new SizeofExpr();
    copy->loc = this->loc;
    if (this->targetType) copy->targetType = this->targetType->cloneAs<TypeNode>();
    return copy;
}
ASTNode* AlignofExpr::cloneImpl() const { 
    auto copy = new AlignofExpr();
    copy->loc = this->loc;
    if (this->targetType) copy->targetType = this->targetType->cloneAs<TypeNode>();
    return copy;
}
"""

    # StmtNode Clones
    # IfStmtNode
    out += """
ASTNode* IfStmtNode::cloneImpl() const {
    auto copy = new IfStmtNode();
    copy->loc = this->loc;
    if (this->condition) copy->condition = this->condition->cloneAs<ExprNode>();
    if (this->thenBranch) copy->thenBranch = this->thenBranch->cloneAs<BlockStmtNode>();
    if (this->elseBranch) copy->elseBranch = this->elseBranch->cloneAs<StmtNode>();
    return copy;
}
"""
    # WhileStmtNode
    out += """
ASTNode* WhileStmtNode::cloneImpl() const {
    auto copy = new WhileStmtNode();
    copy->loc = this->loc;
    if (this->condition) copy->condition = this->condition->cloneAs<ExprNode>();
    if (this->body) copy->body = this->body->cloneAs<BlockStmtNode>();
    return copy;
}
"""
    # ForStmtNode
    out += """
ASTNode* ForStmtNode::cloneImpl() const {
    auto copy = new ForStmtNode();
    copy->loc = this->loc;
    copy->kind = this->kind;
    copy->bindingName = this->bindingName;
    copy->bindingId = this->bindingId;
    if (this->iterable) copy->iterable = this->iterable->cloneAs<ExprNode>();
    if (this->init) copy->init = this->init->cloneAs<ItemNode>();
    if (this->cond) copy->cond = this->cond->cloneAs<ExprNode>();
    if (this->step) copy->step = this->step->cloneAs<ExprNode>();
    if (this->body) copy->body = this->body->cloneAs<BlockStmtNode>();
    copy->bodyScopeId = kInvalidSymbolID;
    return copy;
}
"""
    # Break, Continue, Unsafe, Comptime
    out += """
ASTNode* BreakStmtNode::cloneImpl() const { 
    auto copy = new BreakStmtNode();
    copy->loc = this->loc;
    return copy;
}
ASTNode* ContinueStmtNode::cloneImpl() const { 
    auto copy = new ContinueStmtNode();
    copy->loc = this->loc;
    return copy;
}
ASTNode* UnsafeStmtNode::cloneImpl() const {
    auto copy = new UnsafeStmtNode();
    copy->loc = this->loc;
    if (this->body) copy->body = this->body->cloneAs<BlockStmtNode>();
    return copy;
}
ASTNode* ComptimeStmtNode::cloneImpl() const {
    auto copy = new ComptimeStmtNode();
    copy->loc = this->loc;
    if (this->body) copy->body = this->body->cloneAs<BlockStmtNode>();
    return copy;
}
"""
    with open('d:/fdlang/src/AST/CloneImpl.cpp', 'r') as f:
        content = f.read()
    
    idx2 = content.rfind('} // namespace fl')
    if idx2 != -1:
        content = content[:idx2]
    
    with open('d:/fdlang/src/AST/CloneImpl.cpp', 'w') as f:
        f.write(content + out + "\n} // namespace fl\n")

generate_clone_impls()
print("Done")

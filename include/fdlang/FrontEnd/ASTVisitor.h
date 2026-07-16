#pragma once

namespace fl {

class ProgramNode;

// --- Nhóm ExprNode ---
class NumberExpr;
class IdentifierExpr;
class BinaryExpr;   // MỚI: Phép toán (+, -, *, /, <, >...)
class BooleanExpr;  // MỚI: Từ khóa logic (true/false)

// --- Nhóm StmtNode ---
class VarDeclStmt;
class AssignStmtNode; // MỚI: Gán giá trị
class BlockStmtNode;  // MỚI: Khối lệnh { ... }
class IfStmtNode;     // MỚI: Rẽ nhánh if/else
class WhileStmtNode;  // MỚI: Vòng lặp while
class PrintStmtNode;  // MỚI: Lệnh in

class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;

    virtual void visit(ProgramNode* node) = 0;
    
    // Expr
    virtual void visit(NumberExpr* node) = 0;
    virtual void visit(IdentifierExpr* node) = 0;
    virtual void visit(BinaryExpr* node) = 0;
    virtual void visit(BooleanExpr* node) = 0;

    // Stmt
    virtual void visit(VarDeclStmt* node) = 0;
    virtual void visit(AssignStmtNode* node) = 0;
    virtual void visit(BlockStmtNode* node) = 0;
    virtual void visit(IfStmtNode* node) = 0;
    virtual void visit(WhileStmtNode* node) = 0;
    virtual void visit(PrintStmtNode* node) = 0;
};

} // namespace fl
#pragma once
#include "fdlang/Frontend/ASTVisitor.h"
#include "fdlang/Frontend/SymbolTable.h"

namespace fl {

class SemanticAnalyzer : public ASTVisitor {
private:
    SymbolTable symbolTable;

public:
    SemanticAnalyzer() = default;
    const SymbolTable& getSymbolTable() const { return symbolTable; }

    // --- Cấp cao nhất ---
    void visit(ProgramNode* node) override;

    // --- Nhóm Biểu thức (Expressions) ---
    void visit(NumberExpr* node) override;
    void visit(IdentifierExpr* node) override;
    void visit(BinaryExpr* node) override;
    void visit(BooleanExpr* node) override;

    // --- Nhóm Câu lệnh (Statements) ---
    void visit(VarDeclStmt* node) override;
    void visit(AssignStmtNode* node) override;
    void visit(BlockStmtNode* node) override;
    void visit(IfStmtNode* node) override;
    void visit(WhileStmtNode* node) override;
    void visit(PrintStmtNode* node) override;
};

} // namespace fl
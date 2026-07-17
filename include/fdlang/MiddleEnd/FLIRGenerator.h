// =============================================================================
// fdlang/MiddleEnd/FLIRGenerator.h
//
// FLIRGenerator — AST to FLIR lowering pass.
//
// Responsibilities:
//   - Translate the type-checked AST into the FLIR representation.
//   - Preserve basic blocks and control flow (if/while).
//   - Generate non-SSA allocations (alloca/load/store) for variables.
//
// Preconditions:
//   - Resolver has run (symbolId populated).
//   - TypeChecker has run (inferredType populated).
//
// Outputs:
//   - A complete flir::Module.
// =============================================================================

#pragma once

#include "fdlang/FrontEnd/ASTVisitor.h"
#include "fdlang/MiddleEnd/SymbolTable.h"
#include "fdlang/MiddleEnd/TypeChecker.h"
#include "fdlang/MiddleEnd/FLIR.h"
#include <unordered_map>

namespace fl {

class FLIRGenerator : public ASTVisitor {
public:
    explicit FLIRGenerator(SymbolTable& symTable, TypeChecker& typeChecker);

    /// Generate FLIR for the given program.
    std::unique_ptr<flir::Module> generate(ProgramNode* program);

    // ── ASTVisitor: Statements ────────────────────────────────────────────────

    void visit(ProgramNode*    node) override;
    void visit(VarDeclStmt*    node) override;
    void visit(AssignStmtNode* node) override;
    void visit(BlockStmtNode*  node) override;
    void visit(IfStmtNode*     node) override;
    void visit(WhileStmtNode*  node) override;
    void visit(PrintStmtNode*  node) override;

    // ── ASTVisitor: Expressions ───────────────────────────────────────────────

    void visit(NumberExpr*     node) override;
    void visit(BooleanExpr*    node) override;
    void visit(IdentifierExpr* node) override;
    void visit(BinaryExpr*     node) override;

private:
    SymbolTable& table_;
    TypeChecker& typeChecker_;

    // ── Context State ────────────────────────────────────────────────────────

    std::unique_ptr<flir::Module> module_;
    flir::Function* currentFunction_ = nullptr;
    flir::BasicBlock* currentBlock_ = nullptr;

    int nextLocalId_ = 0;
    int nextLabelId_ = 0;

    /// Maps a SymbolID (variable declaration) to the FLIR LocalId representing
    /// its alloca'd pointer.
    std::unordered_map<SymbolID, flir::LocalId> varAllocas_;

    /// Temporary storage for the operand evaluated by the last expression.
    flir::Operand lastEvaluatedOperand_ = flir::Number{"0"};

    // ── Helpers ──────────────────────────────────────────────────────────────

    flir::LocalId nextLocal();
    flir::LabelId nextLabel(const std::string& prefix = "bb");

    /// Seal the current block with a terminator, if it isn't already terminated.
    void terminateBlock(std::unique_ptr<flir::Terminator> term);

    /// Start a new basic block and make it current.
    void startBlock(flir::LabelId label);
    
    /// Evaluate an expression and return its result operand.
    flir::Operand evaluate(ExprNode* expr);
};

} // namespace fl

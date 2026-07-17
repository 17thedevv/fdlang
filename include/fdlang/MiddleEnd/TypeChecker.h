// =============================================================================
// fdlang/MiddleEnd/TypeChecker.h
//
// TypeChecker — Phase 1 MVP type-checking pass.
//
// Single responsibility:
//   Infer and verify types for all expressions and statements in an
//   already-resolved AST. Annotate every ExprNode::inferredType.
//   Populate a TypeTable (side-table keyed by SymbolID) for downstream passes.
//
// Preconditions (must run AFTER Resolver):
//   - Every IdentifierExpr, VarDeclStmt, AssignStmtNode has symbolId set.
//   - SymbolTable is fully populated; symbolCount() is final.
//
// Does NOT:
//   ✗ Perform name lookup        — all symbolIds are already filled by Resolver
//   ✗ Manage scope stack         — SymbolIDs are unique across scopes
//   ✗ Check ownership/borrows    → BorrowChecker (future)
//   ✗ Generate IR                → FLIRGenerator (future)
//   ✗ Perform type conversions   — no implicit casts in fdlang
//
// Outputs (after check() returns):
//   - typeTable_[symbolId] → FLType for every declared variable
//   - ExprNode::inferredType annotated on every expression in the AST
//   - DiagnosticEngine populated with all type errors (flushed by caller)
//
// Side-table design (see Symbol.h rationale):
//   Type information is stored in a separate TypeTable vector, not inside
//   Symbol. This keeps Symbol as identity-only metadata (id, name, location,
//   decl*) and avoids coupling the type system into the name resolution layer.
//
// TODO(BorrowChecker): Expose isInitialized(SymbolID) here so BorrowChecker
//   can read definite-initialization state without recomputing it.
// TODO(functions): When FuncDecl arrives, TypeChecker will maintain a
//   return-type context stack to verify return statement types.
// TODO(generics): When generic types arrive, a unification/substitution step
//   will be needed before this pass. FLType::Unknown will be repurposed or
//   replaced with a proper type variable representation.
// =============================================================================

#pragma once
#include "fdlang/FrontEnd/ASTVisitor.h"
#include "fdlang/Core/FLType.h"
#include "fdlang/Core/Types.h"
#include "fdlang/Core/SourceLocation.h"
#include "fdlang/MiddleEnd/SymbolTable.h"
#include "fdlang/Support/Diagnostic.h"
#include "fdlang/AST/ASTNode.h"
#include <vector>
#include <string_view>
#include <cstddef>

// Forward declarations — included fully in TypeChecker.cpp
namespace fl {
class ProgramNode;
class VarDeclStmt;
class AssignStmtNode;
class BlockStmtNode;
class IfStmtNode;
class WhileStmtNode;
class PrintStmtNode;
class NumberExpr;
class IdentifierExpr;
class BinaryExpr;
class BooleanExpr;
class ExprNode;
}

namespace fl {

class TypeChecker : public ASTVisitor {
public:
    // ── Construction ─────────────────────────────────────────────────────────

    /// @param table  Post-Resolver SymbolTable. Must outlive this TypeChecker.
    /// @param diag   DiagnosticEngine (single source of truth for errors).
    ///               Must outlive this TypeChecker. Mandatory reference.
    explicit TypeChecker(SymbolTable& table, DiagnosticEngine& diag);

    // ── Entry Point ───────────────────────────────────────────────────────────

    /// Run the type-checking pass over the fully-resolved program AST.
    /// Sizes the internal side-tables to table_.symbolCount() before traversal.
    ///
    /// @return true  if no type errors were reported.
    ///         false if any type error was reported to diag_.
    bool check(ProgramNode* program);

    // ── Query (for tests and FLIR) ────────────────────────────────────────────

    /// Returns the inferred type of the symbol with the given ID.
    /// Valid only after check() has been called.
    FLType typeOf(SymbolID id) const;

    /// Returns true if the symbol has been definitely initialized.
    /// Valid only after check() has been called.
    /// Exposed for future BorrowChecker to consume without re-tracking.
    bool isInitialized(SymbolID id) const;

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
    // ── Members ──────────────────────────────────────────────────────────────

    SymbolTable&      table_;
    DiagnosticEngine& diag_;     ///< Single source of truth for error counting.

    /// Side table: symbolId → inferred FLType.
    /// Indexed by SymbolID (index into SymbolTable::arena_).
    /// Sized to table_.symbolCount() at the start of check().
    /// Initialized to FLType::Unknown for all symbols.
    std::vector<FLType> typeTable_;

    /// Side table: symbolId → definitely initialized flag.
    /// Indexed by SymbolID. Set to true when:
    ///   - VarDeclStmt has an initializer expression.
    ///   - AssignStmtNode writes to the symbol (first or subsequent assignment).
    /// Read by visit(IdentifierExpr*) to detect use-before-initialization.
    std::vector<bool> initialized_;

    // ── Private Helpers ───────────────────────────────────────────────────────

    /// Visit an expression and return its inferred type.
    /// Sets node->inferredType as a side-effect for downstream passes (FLIR).
    FLType inferExpr(ExprNode* expr);

    /// Infer the result type of a binary operation given the operand types.
    /// Reports an error via diag_ and returns FLType::Unknown on invalid combos.
    FLType inferBinaryOp(std::string_view op,
                         FLType           left,
                         FLType           right,
                         SourceLocation   loc);

    /// Visit expr and check that its inferred type equals `required`.
    /// Reports a type mismatch error if not.
    void requireType(ExprNode*        expr,
                     FLType           required,
                     std::string_view context);

    /// Report a type mismatch error to the DiagnosticEngine.
    void reportTypeMismatch(SourceLocation   loc,
                            std::string_view context,
                            FLType           expected,
                            FLType           got);
};

} // namespace fl

// =============================================================================
// fdlang/MiddleEnd/Resolver.h
//
// Resolver — the Name Resolution pass.
//
// Single responsibility:
//   Connect every identifier USE to its DECLARATION by annotating AST nodes
//   with SymbolIDs. That is ALL this pass does.
//
// Does NOT:
//   ✗ Check if a variable was initialized before use  → SemanticAnalyzer
//   ✗ Check types (int + string)                      → TypeChecker
//   ✗ Check ownership / borrow rules                  → BorrowChecker
//   ✗ Evaluate constant expressions                   → ConstEvaluator
//   ✗ Generate IR                                     → IRGenerator
//
// Exactly four responsibilities:
//   1. Scope management   — enterScope() / exitScope() driven by AST traversal
//   2. Declare symbols    — VarDeclStmt → create Symbol in SymbolTable
//   3. Resolve identifiers — IdentifierExpr.name → IdentifierExpr.symbolId
//   4. Shadowing support  — inner scope declarations shadow outer ones
//
// Output:
//   - SymbolTable fully populated with all Symbols and Scopes.
//   - All IdentifierExpr, VarDeclStmt, AssignStmtNode nodes have symbolId set.
//   - errorCount() > 0 if any unresolved names or redeclarations were found.
//
// Architecture comparison:
//   rustc:  resolve/mod.rs — strict separation from type inference (ty/ crate)
//   Clang:  IdentifierResolver — lookup chains through DeclContexts
//   Swift:  NameBinder — AST-walking pass that annotates DeclRefExpr nodes
//
//   fdlang follows rustc's philosophy: each pass has exactly one job.
// =============================================================================

#pragma once
#include "fdlang/MiddleEnd/SymbolTable.h"
#include "fdlang/MiddleEnd/ScopeStack.h"
#include "fdlang/FrontEnd/ASTVisitor.h"
#include "fdlang/Core/SourceLocation.h"
#include "fdlang/AST/ASTNode.h"
#include "fdlang/Support/Diagnostic.h"
#include <cstddef>
#include <string>
#include <string_view>

// Forward declarations — included fully in Resolver.cpp
namespace fl {
class ProgramNode;
class VarDeclStmt;
class AssignStmtNode;
class PrintStmtNode;
class BlockStmtNode;
class IfStmtNode;
class WhileStmtNode;
class IdentifierExpr;
class BinaryExpr;
class NumberExpr;
class BooleanExpr;
}

namespace fl {

class Resolver : public ASTVisitor {
public:
    // ── Construction ─────────────────────────────────────────────────────────

    /// @param table  The SymbolTable to populate. Must outlive this Resolver.
    /// @param diag   The DiagnosticEngine to receive all error/warning reports.
    ///               Must outlive this Resolver. Mandatory — no nullptr allowed.
    explicit Resolver(SymbolTable& table, DiagnosticEngine& diag);

    // ── Entry Point ──────────────────────────────────────────────────────────

    /// Resolve all names in the given program.
    /// Traverses the entire AST exactly once.
    ///
    /// @return true  if resolution succeeded with zero errors.
    ///         false if any undeclared name or redeclaration was found.
    bool resolve(ProgramNode* program);

    /// Number of resolution errors — delegates to the DiagnosticEngine.
    /// DiagnosticEngine is the single source of truth for error counting.
    size_t errorCount() const { return diag_.errorCount(); }

    // ── ASTVisitor: Statements ────────────────────────────────────────────────

    void visit(ProgramNode*    node) override;
    void visit(VarDeclStmt*    node) override;
    void visit(AssignStmtNode* node) override;
    void visit(PrintStmtNode*  node) override;
    void visit(BlockStmtNode*  node) override;
    void visit(IfStmtNode*     node) override;
    void visit(WhileStmtNode*  node) override;

    // ── ASTVisitor: Expressions ───────────────────────────────────────────────

    void visit(IdentifierExpr* node) override;
    void visit(BinaryExpr*     node) override;
    void visit(NumberExpr*     node) override;
    void visit(BooleanExpr*    node) override;

private:
    // ── Members ──────────────────────────────────────────────────────────────

    SymbolTable&      table_;
    DiagnosticEngine& diag_;      ///< Single source of truth for error counting.
    ScopeStack        scopeStack_;

    // ── Private Helpers ───────────────────────────────────────────────────────

    /// Open a new scope of the given kind and push it onto the active stack.
    void enterScope(ScopeKind kind);

    /// Close the current scope and pop it from the active stack.
    void exitScope();

    /// Declare a name in the CURRENT scope.
    ///
    /// Checks for redeclaration in the current scope only (not the full chain).
    /// This correctly allows shadowing: inner `dec x` is OK even if outer `dec x` exists.
    ///
    /// @param name     Source-text name.
    /// @param kind     SymbolKind of the new symbol.
    /// @param loc      Source location for diagnostics.
    /// @param declNode Back-pointer to the declaring AST node.
    /// @return         New SymbolID, or kInvalidSymbolID on redeclaration error.
    SymbolID declare(std::string_view name,
                     SymbolKind       kind,
                     SourceLocation   loc,
                     ASTNode*         declNode);

    /// Resolve a name by walking the scope chain from the current scope outward.
    ///
    /// @param name Source-text name to look up.
    /// @param loc  Source location for diagnostic messages.
    /// @return     SymbolID of the matching declaration,
    ///             or kInvalidSymbolID if not found (error reported internally).
    SymbolID resolveId(std::string_view name, SourceLocation loc);

    /// Report a resolution error via the DiagnosticEngine.
    /// DiagnosticEngine is the single source of truth — Resolver owns no counters.
    void reportError(SourceLocation loc, const std::string& message);
};

} // namespace fl

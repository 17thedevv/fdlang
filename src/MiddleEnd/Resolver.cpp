#include "fdlang/MiddleEnd/Resolver.h"
#include "fdlang/AST/ASTNode.h"
#include "fdlang/AST/ExprNode.h"
#include "fdlang/AST/StmtNode.h"
#include <cassert>

namespace fl {

// =============================================================================
// Construction
// =============================================================================

Resolver::Resolver(SymbolTable& table, DiagnosticEngine& diag)
    : table_(table), diag_(diag) {}

// =============================================================================
// Entry Point
// =============================================================================

bool Resolver::resolve(ProgramNode* program) {
    assert(program != nullptr && "Resolver::resolve() called with null ProgramNode");
    // Note: we do NOT assert errorCount==0 here because the engine is shared;
    // a future CompilerContext may run multiple passes against the same engine.

    // The global scope (ScopeID 0) was created by SymbolTable's constructor.
    // Push it as the starting scope for this traversal.
    scopeStack_.push(table_.globalScopeId());

    program->accept(this);

    scopeStack_.pop();

    // Post-condition: all scopes should be closed.
    assert(scopeStack_.isEmpty() &&
           "ScopeStack not empty after resolve — mismatched enter/exitScope");

    return !diag_.hasErrors();
}

// =============================================================================
// Private Helpers
// =============================================================================

void Resolver::enterScope(ScopeKind kind) {
    ScopeID parentId = scopeStack_.current();
    ScopeID newScope = table_.createScope(kind, parentId);
    scopeStack_.push(newScope);
}

void Resolver::exitScope() {
    scopeStack_.pop();
}

SymbolID Resolver::declare(std::string_view name,
                            SymbolKind       kind,
                            SourceLocation   loc,
                            ASTNode*         declNode) {
    ScopeID currentScope = scopeStack_.current();

    // --- Redeclaration check ---
    // Check ONLY the current scope (not the full chain).
    // This is what allows shadowing:
    //   dec x = 1;
    //   { dec x = 2; }   ← legal: inner x shadows outer x
    //
    // If we walked the full chain here, shadowing would be rejected.
    if (table_.containsInScope(name, currentScope)) {
        reportError(loc, "Bien '" + std::string(name) +
                    "' da duoc khai bao trong scope nay.");
        return kInvalidSymbolID;
    }

    Identifier ident(name);
    return table_.declareSymbol(ident, kind, currentScope, loc, declNode);
}

SymbolID Resolver::resolveId(std::string_view name, SourceLocation loc) {
    ScopeID currentScope = scopeStack_.current();

    // Walk the scope chain from innermost outward.
    // The first match wins — this implements shadowing correctly.
    auto result = table_.lookup(name, currentScope);
    if (!result.has_value()) {
        reportError(loc, "Bien '" + std::string(name) + "' chua duoc khai bao.");
        return kInvalidSymbolID;
    }

    return result.value();
}

void Resolver::reportError(SourceLocation loc, const std::string& message) {
    diag_.error(loc, message);
}

// =============================================================================
// ASTVisitor: Statements
// =============================================================================

void Resolver::visit(ProgramNode* node) {
    for (auto& stmt : node->statements) {
        stmt->accept(this);
    }
}

void Resolver::visit(VarDeclStmt* node) {
    // --- Order matters: resolve initializer BEFORE declaring the variable ---
    //
    // This correctly rejects:
    //   dec x = x;     ← x used in its own initializer, not yet declared
    //
    // And correctly accepts:
    //   dec x = 10;
    //   dec y = x + 1; ← x is already declared, found in scope
    //
    // (rustc follows the same order in its resolve pass)
    if (node->initializer) {
        node->initializer->accept(this);
    }

    // Declare the variable into the current scope.
    // MVP: SourceLocation carries only offset=0 because the Parser does not
    // currently track byte offsets on declaration nodes.
    // Future: Parser sets node->location from the Token's byteOffset.
    SourceLocation loc;   // MVP: all fields default to 0
    SymbolID id = declare(node->varName, SymbolKind::Variable, loc, node);

    // Annotate the AST node (read by TypeChecker, BorrowChecker, IRGenerator).
    node->symbolId = id;
}

void Resolver::visit(AssignStmtNode* node) {
    // 1. Resolve the right-hand side first (consistent left-to-right order).
    node->value->accept(this);

    // 2. Resolve the target variable.
    //    Assignment does NOT introduce a new name — it must already exist.
    SourceLocation loc;
    SymbolID id = resolveId(node->varName, loc);
    node->symbolId = id;
}

void Resolver::visit(BlockStmtNode* node) {
    // Every { } opens a new Block scope.
    // The scope is created, all statements inside are resolved,
    // then the scope is closed. Any variables declared inside are
    // no longer visible after exitScope().
    enterScope(ScopeKind::Block);

    for (auto& stmt : node->statements) {
        stmt->accept(this);
    }

    exitScope();
}

void Resolver::visit(IfStmtNode* node) {
    // Resolve the condition in the current scope.
    node->condition->accept(this);

    // thenBranch is a BlockStmtNode — it opens its own scope internally.
    node->thenBranch->accept(this);

    // elseBranch (if present) also opens its own scope.
    if (node->elseBranch) {
        node->elseBranch->accept(this);
    }

    // Design note: we do NOT open an extra Conditional scope here because
    // the condition expression itself does not declare any names in MVP.
    // Future: pattern matching `if let x = expr { }` will need a scope
    // opened around the condition+then-branch.
}

void Resolver::visit(WhileStmtNode* node) {
    // Resolve condition in the current scope.
    node->condition->accept(this);

    // Body is a BlockStmtNode — manages its own Block scope.
    // Variables declared inside the loop body are destroyed each iteration
    // and are not visible after the loop.
    //
    // Future: insert ScopeKind::Loop here (between current scope and body's
    // Block scope) to hold break/continue resolution targets.
    node->body->accept(this);
}

void Resolver::visit(PrintStmtNode* node) {
    // Resolver does NOT create any symbol for 'print'.
    // 'print' is a built-in statement node in the MVP AST.
    //
    // The ONLY job here: resolve the argument expression, so that any
    // identifiers inside (e.g., `print x;`) get their symbolId filled in.
    //
    // Migration note: when 'print' becomes a stdlib function, the Parser
    // will emit FuncCallExpr("print", [arg]) instead of PrintStmtNode.
    // The IdentifierExpr("print") will be resolved naturally here.
    // PrintStmtNode can then be deleted with zero Resolver changes.
    node->value->accept(this);
}

// =============================================================================
// ASTVisitor: Expressions
// =============================================================================

void Resolver::visit(IdentifierExpr* node) {
    // The core operation of name resolution:
    // Connect this usage of a name to the Symbol of its declaration.
    SourceLocation loc;
    SymbolID id = resolveId(node->name, loc);
    node->symbolId = id;   // annotate the AST node
}

void Resolver::visit(BinaryExpr* node) {
    // Recurse into both operands.
    // The binary operator itself has no symbol to resolve.
    node->left->accept(this);
    node->right->accept(this);
}

void Resolver::visit(NumberExpr* /*node*/) {
    // Number literals introduce no names. Nothing to resolve.
}

void Resolver::visit(BooleanExpr* /*node*/) {
    // Boolean literals (true/false) introduce no names. Nothing to resolve.
}

} // namespace fl

#pragma once
#include "ASTNode.h"
#include "ExprNode.h"
#include "fdlang/FrontEnd/ASTVisitor.h"   // fixed: FrontEnd (capital E)
#include "fdlang/Core/Types.h"            // SymbolID, kInvalidSymbolID
#include <memory>
#include <string_view>
#include <vector>

namespace fl {

class StmtNode : public ASTNode {};

// =============================================================================
// DeclNode — base class for ALL declarations in fdlang.
//
// Design rationale (inspired by Clang's Decl hierarchy):
//   Every language construct that introduces a new name into a scope
//   (variable, constant, function, struct, enum, module, trait, parameter)
//   should be a DeclNode subclass.
//
//   Benefits:
//     - symbolId in one place: TypeChecker / BorrowChecker / IRGenerator
//       can do `auto* decl = dynamic_cast<DeclNode*>(stmt)` and get symbolId.
//     - Uniform traversal: a future DeclVisitor can specialize on declarations.
//     - Extensible: adding FuncDecl, StructDecl, etc. just adds a DeclNode
//       subclass — no changes to existing infrastructure.
//
//   MVP declarations:
//     - VarDeclStmt  (dec x = ...)
//
//   Future declarations:
//     - FuncDecl     (fn foo(...) {...})
//     - StructDecl   (struct Foo {...})
//     - EnumDecl     (enum Bar {...})
//     - ModuleDecl   (mod m {...})
//     - ParamDecl    (fn foo(x: i32))
//
// Note: DeclNode inherits StmtNode so the Parser's parseStatement()
// return type (unique_ptr<StmtNode>) is compatible without changes.
// =============================================================================
class DeclNode : public StmtNode {
public:
    // ── Resolver annotation ──────────────────────────────────────────────────
    // Default: kInvalidSymbolID (not yet resolved).
    // After Resolver pass: index into SymbolTable::arena_.
    SymbolID symbolId = kInvalidSymbolID;
};

// =============================================================================
// VarDeclStmt — variable declaration: dec x = expr;
// =============================================================================
class VarDeclStmt : public DeclNode {
public:
    std::string_view varName;
    std::unique_ptr<ExprNode> initializer;   // nullable (dec x; has no init)

    VarDeclStmt(std::string_view name, std::unique_ptr<ExprNode> init)
        : varName(name), initializer(std::move(init)) {}

    void accept(ASTVisitor* visitor) override {
        visitor->visit(this);
    }
};

// =============================================================================
// AssignStmtNode — assignment: x = expr;
//
// Not a DeclNode (assignment does NOT introduce a new name).
// Carries symbolId to record WHICH variable is being assigned to,
// enabling downstream passes (BorrowChecker, IRGenerator) to find
// the original declaration without a second name lookup.
// =============================================================================
class AssignStmtNode : public StmtNode {
public:
    std::string_view varName;
    std::unique_ptr<ExprNode> value;

    // ── Resolver annotation ──────────────────────────────────────────────────
    // Filled by Resolver: points to the VarDeclStmt's Symbol.
    SymbolID symbolId = kInvalidSymbolID;

    AssignStmtNode(std::string_view name, std::unique_ptr<ExprNode> val)
        : varName(name), value(std::move(val)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

// =============================================================================
// BlockStmtNode — block of statements: { stmt* }
// Opens a new lexical scope. Managed by Resolver::visit(BlockStmtNode*).
// =============================================================================
class BlockStmtNode : public StmtNode {
public:
    std::vector<std::unique_ptr<StmtNode>> statements;

    explicit BlockStmtNode(std::vector<std::unique_ptr<StmtNode>> stmts)
        : statements(std::move(stmts)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

// =============================================================================
// IfStmtNode — conditional: if (cond) { } else { }
// Each branch is a BlockStmtNode and manages its own scope.
// =============================================================================
class IfStmtNode : public StmtNode {
public:
    std::unique_ptr<ExprNode> condition;
    std::unique_ptr<StmtNode> thenBranch;
    std::unique_ptr<StmtNode> elseBranch;   // nullptr if no else

    IfStmtNode(std::unique_ptr<ExprNode> cond,
               std::unique_ptr<StmtNode> thenB,
               std::unique_ptr<StmtNode> elseB = nullptr)
        : condition(std::move(cond)),
          thenBranch(std::move(thenB)),
          elseBranch(std::move(elseB)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

// =============================================================================
// WhileStmtNode — loop: while (cond) { }
// Body is a BlockStmtNode managing its own scope.
// Future: insert ScopeKind::Loop scope here for break/continue resolution.
// =============================================================================
class WhileStmtNode : public StmtNode {
public:
    std::unique_ptr<ExprNode> condition;
    std::unique_ptr<StmtNode> body;

    WhileStmtNode(std::unique_ptr<ExprNode> cond, std::unique_ptr<StmtNode> bodyStmt)
        : condition(std::move(cond)), body(std::move(bodyStmt)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

// =============================================================================
// PrintStmtNode — built-in print statement: print expr;
//
// MVP: 'print' is a language keyword, not a function.
// Resolver does NOT assign a SymbolID to 'print' itself.
// Resolver only resolves the argument expression.
//
// Migration path to stdlib:
//   When 'print' moves to a standard library function, the Parser will
//   produce a FuncCallExpr("print", args) instead of PrintStmtNode.
//   The IdentifierExpr("print") inside the call will be resolved naturally
//   by Resolver. PrintStmtNode can then be removed with zero Resolver changes.
// =============================================================================
class PrintStmtNode : public StmtNode {
public:
    std::unique_ptr<ExprNode> value;

    explicit PrintStmtNode(std::unique_ptr<ExprNode> val) : value(std::move(val)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

} // namespace fl

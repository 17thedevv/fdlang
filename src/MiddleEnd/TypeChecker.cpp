// =============================================================================
// fdlang/MiddleEnd/TypeChecker.cpp
//
// TypeChecker implementation — Phase 1 MVP.
//
// FORMATTING NOTE (mirroring Diagnostic.cpp convention):
//   All diagnostic message text lives in the report* helpers and in
//   inferBinaryOp(). No raw string literals scattered across visit() methods.
//   Future localization or message-code numbering changes only those helpers.
// =============================================================================

#include "fdlang/MiddleEnd/TypeChecker.h"
#include "fdlang/AST/ASTNode.h"
#include "fdlang/AST/ExprNode.h"
#include "fdlang/AST/StmtNode.h"
#include <cassert>
#include <string>

namespace fl {

// =============================================================================
// Construction
// =============================================================================

TypeChecker::TypeChecker(SymbolTable& table, DiagnosticEngine& diag)
    : table_(table), diag_(diag) {}

// =============================================================================
// Entry Point
// =============================================================================

bool TypeChecker::check(ProgramNode* program) {
    assert(program != nullptr &&
           "TypeChecker::check() called with null ProgramNode");

    // Size both side-tables to the final symbol count.
    // After Resolver, symbolCount() is stable — no new symbols are created here.
    size_t n = table_.symbolCount();
    typeTable_.assign(n, FLType::Unknown);
    initialized_.assign(n, false);

    program->accept(this);
    return !diag_.hasErrors();
}

// =============================================================================
// Query
// =============================================================================

FLType TypeChecker::typeOf(SymbolID id) const {
    assert(id < typeTable_.size() &&
           "TypeChecker::typeOf() called before check() or with invalid SymbolID");
    return typeTable_[id];
}

bool TypeChecker::isInitialized(SymbolID id) const {
    assert(id < initialized_.size() &&
           "TypeChecker::isInitialized() called with invalid SymbolID");
    return initialized_[id];
}

// =============================================================================
// Private Helpers
// =============================================================================

FLType TypeChecker::inferExpr(ExprNode* expr) {
    expr->accept(this);
    return expr->inferredType;
}

FLType TypeChecker::inferBinaryOp(std::string_view op,
                                   FLType left,
                                   FLType right,
                                   SourceLocation loc) {
    // ── Arithmetic: Int × Int → Int ───────────────────────────────────────────
    if (op == "+" || op == "-" || op == "*" || op == "/") {
        if (left == FLType::Int && right == FLType::Int) {
            return FLType::Int;
        }
        diag_.error(loc,
            "Toan tu '" + std::string(op) + "' yeu cau hai toan hang kieu 'int', "
            "nhung nhan duoc '" + std::string(flTypeName(left)) +
            "' va '" + std::string(flTypeName(right)) + "'.");
        return FLType::Unknown;
    }

    // ── Relational: Int × Int → Bool ─────────────────────────────────────────
    if (op == "<" || op == ">" || op == "<=" || op == ">=") {
        if (left == FLType::Int && right == FLType::Int) {
            return FLType::Bool;
        }
        diag_.error(loc,
            "Toan tu '" + std::string(op) + "' yeu cau hai toan hang kieu 'int', "
            "nhung nhan duoc '" + std::string(flTypeName(left)) +
            "' va '" + std::string(flTypeName(right)) + "'.");
        return FLType::Unknown;
    }

    // ── Equality: T × T → Bool (same concrete type only) ────────────────────
    if (op == "==" || op == "!=") {
        if (left == right && left != FLType::Unknown) {
            return FLType::Bool;
        }
        diag_.error(loc,
            "Toan tu '" + std::string(op) + "' yeu cau hai toan hang cung kieu, "
            "nhung nhan duoc '" + std::string(flTypeName(left)) +
            "' va '" + std::string(flTypeName(right)) + "'.");
        return FLType::Unknown;
    }

    // ── Logical: Bool × Bool → Bool ──────────────────────────────────────────
    // TODO(future): && and || — add when parser emits BinaryExpr for them.
    if (op == "&&" || op == "||") {
        if (left == FLType::Bool && right == FLType::Bool) {
            return FLType::Bool;
        }
        diag_.error(loc,
            "Toan tu '" + std::string(op) + "' yeu cau hai toan hang kieu 'bool', "
            "nhung nhan duoc '" + std::string(flTypeName(left)) +
            "' va '" + std::string(flTypeName(right)) + "'.");
        return FLType::Unknown;
    }

    // Unknown operator — should not happen with current grammar.
    diag_.error(loc,
        "Toan tu khong duoc ho tro trong MVP: '" + std::string(op) + "'.");
    return FLType::Unknown;
}

void TypeChecker::requireType(ExprNode*        expr,
                               FLType           required,
                               std::string_view context) {
    FLType actual = inferExpr(expr);
    if (actual != required) {
        // MVP: SourceLocation not yet tracked by Lexer on expression nodes.
        reportTypeMismatch(SourceLocation{}, context, required, actual);
    }
}

void TypeChecker::reportTypeMismatch(SourceLocation   loc,
                                      std::string_view context,
                                      FLType           expected,
                                      FLType           got) {
    diag_.error(loc,
        std::string(context) + ": kieu du lieu khong khop. "
        "Mong doi '" + std::string(flTypeName(expected)) +
        "', nhung nhan duoc '" + std::string(flTypeName(got)) + "'.");
}

// =============================================================================
// ASTVisitor: Statements
// =============================================================================

void TypeChecker::visit(ProgramNode* node) {
    for (auto& stmt : node->statements) {
        stmt->accept(this);
    }
}

void TypeChecker::visit(VarDeclStmt* node) {
    if (node->symbolId == kInvalidSymbolID) {
        // Resolver reported an error for this declaration.
        // Skip: we have no valid symbol to annotate.
        return;
    }

    if (node->initializer) {
        // Infer the type from the initializer expression.
        FLType initType = inferExpr(node->initializer.get());
        typeTable_[node->symbolId]    = initType;
        initialized_[node->symbolId] = true;
    } else {
        // dec x; — no initializer.
        // Type stays Unknown until the first assignment (Q2: promotion allowed).
        typeTable_[node->symbolId]    = FLType::Unknown;
        initialized_[node->symbolId] = false;
    }
}

void TypeChecker::visit(AssignStmtNode* node) {
    if (node->symbolId == kInvalidSymbolID) {
        // Resolver reported an error — skip to avoid cascading type errors.
        return;
    }

    // Infer the right-hand side type first.
    FLType rhsType = inferExpr(node->value.get());

    FLType& symType = typeTable_[node->symbolId];

    if (symType == FLType::Unknown) {
        // Q2: Unknown → concrete type promotion on first assignment.
        // This handles `dec x; x = 5;` — x becomes Int.
        symType = rhsType;
    } else if (rhsType != FLType::Unknown && symType != rhsType) {
        // Type mismatch: cannot assign rhsType to a variable of symType.
        const Symbol& sym = table_.getSymbol(node->symbolId);
        diag_.error(SourceLocation{},
            "Khong the gan gia tri kieu '" + std::string(flTypeName(rhsType)) +
            "' cho bien '" + sym.name.str() +
            "' kieu '" + std::string(flTypeName(symType)) + "'.");
    }

    // Mark as initialized regardless of type error — assignment IS initialization.
    // This prevents spurious "use before init" errors on subsequent uses.
    initialized_[node->symbolId] = true;
}

void TypeChecker::visit(BlockStmtNode* node) {
    // TypeChecker does NOT manage a scope stack.
    // SymbolIDs are globally unique — side-table access is always correct.
    for (auto& stmt : node->statements) {
        stmt->accept(this);
    }
}

void TypeChecker::visit(IfStmtNode* node) {
    // Condition must be Bool.
    requireType(node->condition.get(), FLType::Bool, "Dieu kien if");

    node->thenBranch->accept(this);
    if (node->elseBranch) {
        node->elseBranch->accept(this);
    }
}

void TypeChecker::visit(WhileStmtNode* node) {
    // Condition must be Bool.
    requireType(node->condition.get(), FLType::Bool, "Dieu kien while");
    node->body->accept(this);
}

void TypeChecker::visit(PrintStmtNode* node) {
    // print accepts any type — just infer to catch nested errors (e.g., use-before-init).
    // TODO(functions): when print migrates to stdlib, remove this node and
    // the special case disappears naturally via FuncCallExpr resolution.
    inferExpr(node->value.get());
}

// =============================================================================
// ASTVisitor: Expressions
// =============================================================================

void TypeChecker::visit(NumberExpr* node) {
    // All number literals are Int in MVP.
    // TODO(types): when float literals are added, parse the value string to distinguish.
    node->inferredType = FLType::Int;
}

void TypeChecker::visit(BooleanExpr* node) {
    node->inferredType = FLType::Bool;
}

void TypeChecker::visit(IdentifierExpr* node) {
    if (node->symbolId == kInvalidSymbolID) {
        // Resolver error — propagate Unknown to suppress cascading type errors.
        node->inferredType = FLType::Unknown;
        return;
    }

    // Use-before-initialization check.
    if (!initialized_[node->symbolId]) {
        const Symbol& sym = table_.getSymbol(node->symbolId);
        diag_.error(SourceLocation{},
            "Bien '" + sym.name.str() +
            "' duoc su dung truoc khi duoc khoi tao.");
        // Propagate Unknown so downstream expressions don't cascade into
        // spurious type-mismatch errors on this symbol.
        node->inferredType = FLType::Unknown;
        return;
    }

    node->inferredType = typeTable_[node->symbolId];
}

void TypeChecker::visit(BinaryExpr* node) {
    FLType left  = inferExpr(node->left.get());
    FLType right = inferExpr(node->right.get());

    // MVP: SourceLocation not yet tracked per expression — pass default.
    node->inferredType = inferBinaryOp(node->op, left, right, SourceLocation{});
}

} // namespace fl

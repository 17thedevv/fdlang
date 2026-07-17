// =============================================================================
// fdlang/Core/FLType.h
//
// FLType — the type system for fdlang.
//
// Lives in Core (not MiddleEnd) because it is referenced by multiple layers:
//   - ExprNode::inferredType  (AST layer)
//   - TypeChecker::typeTable_ (MiddleEnd layer)
//   - FLIRGenerator           (BackEnd layer)
//
// Design philosophy:
//   Keep the enum minimal. Add variants only when the corresponding
//   AST nodes and grammar rules exist to generate them.
//
// TODO(types): FLType::Float, FLType::String when literals are added.
// TODO(functions): FLType::Function will need an associated signature
//   representation (parameter types + return type). Not here — a separate
//   FunctionType record will be needed.
// TODO(generics): FLType::Generic(TypeParam) for polymorphic functions.
//   Requires a unification pass (Hindley-Milner style) before TypeChecker runs.
// TODO(user-types): FLType::Named(SymbolID) for structs and enums.
//   SymbolID identifies the defining struct/enum declaration.
// =============================================================================

#pragma once
#include <string_view>
#include <cstdint>

namespace fl {

// =============================================================================
// FLType
// =============================================================================

/// Type tag for every value in fdlang.
///
/// MVP types:
///   Unknown — temporary placeholder for variables declared without initializer
///             (dec x;). Promoted to a concrete type on the first assignment.
///             Not a real type — should not appear in fully-typed IR.
///   Int     — machine integer (dec x = 1;)
///   Bool    — boolean value   (dec b = true;)
enum class FLType : uint8_t {
    Unknown,   ///< Not yet inferred. Valid only transiently during TypeChecker.
    Int,       ///< Integer type. Arithmetic and comparison operators apply.
    Bool,      ///< Boolean type. Logical operators and if/while conditions.
};

// =============================================================================
// flTypeName — single source of truth for type display
// =============================================================================

/// Return the source-level spelling of a type, for diagnostic messages.
/// Used exclusively by DiagnosticEngine formatting — not for code generation.
constexpr std::string_view flTypeName(FLType t) noexcept {
    switch (t) {
        case FLType::Unknown: return "unknown";
        case FLType::Int:     return "int";
        case FLType::Bool:    return "bool";
    }
    return "?"; // unreachable — suppresses compiler warning
}

} // namespace fl

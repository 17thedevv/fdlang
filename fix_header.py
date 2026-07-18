with open('d:/fdlang/include/fdlang/MiddleEnd/TypeChecker.h', 'wb') as f:
    f.write(b"""// =============================================================================
// fdlang/MiddleEnd/TypeChecker.h
//
// TypeChecker - Evaluates and infers Semantic Types.
// =============================================================================
#pragma once
#include "fdlang/FrontEnd/ASTVisitor.h"
#include "fdlang/Core/FLType.h"
#include "fdlang/MiddleEnd/SymbolTable.h"
#include "fdlang/Support/Diagnostic.h"
#include <vector>

namespace fl {

class MonomorphizationEngine;

class TypeChecker {
public:
    explicit TypeChecker(SymbolTable& table, DiagnosticEngine& diag, TypeContext& ctx, MonomorphizationEngine* monoEngine = nullptr);
    bool check(ASTNode* root);
    const Type* typeOf(SymbolID id) const;
    TypeContext& getContext() const { return ctx_; }

private:
    SymbolTable& table_;
    DiagnosticEngine& diag_;
    TypeContext& ctx_;
    MonomorphizationEngine* monoEngine_;
    std::vector<const Type*> typeTable_;
};

} // namespace fl
""")

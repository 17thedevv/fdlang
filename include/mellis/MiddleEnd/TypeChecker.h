// =============================================================================
// mellis/MiddleEnd/TypeChecker.h
//
// TypeChecker - Evaluates and infers Semantic Types.
// =============================================================================
#pragma once
#include "mellis/FrontEnd/ASTVisitor.h"
#include "mellis/Core/FLType.h"
#include "mellis/MiddleEnd/SymbolTable.h"
#include "mellis/Support/Diagnostic.h"
#include "mellis/MLib/MLibMetadataCache.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace fl {

class MonomorphizationEngine;

class TypeChecker {
public:
    explicit TypeChecker(SymbolTable& table, DiagnosticEngine& diag, TypeContext& ctx, MonomorphizationEngine* monoEngine = nullptr);
    bool check(ASTNode* root);
    const Type* typeOf(SymbolID id) const;
    TypeContext& getContext() const { return ctx_; }
    void setMonomorphizationEngine(MonomorphizationEngine* engine) { monoEngine_ = engine; }

    // Attach an MLibMetadataCache so TypeChecker can resolve external symbols.
    // Must be called before check() if any external modules are loaded.
    void setMetadataCache(MLibMetadataCache* cache) { metadataCache_ = cache; }

    void registerImpl(const Type* type, SymbolID traitId);
    bool implementsTrait(const Type* type, SymbolID traitId) const;

    struct MethodInfo {
        SymbolID methodId;
        const FunctionType* type;
    };

    class MethodResolver {
        std::unordered_map<const Type*, std::unordered_map<std::string, MethodInfo>> implMap;
    public:
        void addMethod(const Type* receiverType, const std::string& name, SymbolID methodId, const FunctionType* type, TypeContext& ctx, const Type* selfTypeOverride = nullptr);
        bool probe(const Type* receiverType, const std::string& name, MethodInfo& outMethod);
    };

    MethodResolver& getMethodResolver() { return methodResolver_; }

private:
    SymbolTable& table_;
    DiagnosticEngine& diag_;
    TypeContext& ctx_;
    MonomorphizationEngine* monoEngine_;
    MLibMetadataCache* metadataCache_ = nullptr; // optional, set before check()
    std::vector<const Type*> typeTable_;
    std::unordered_map<const Type*, std::unordered_set<SymbolID>> implementedTraits_;
    MethodResolver methodResolver_;
};


} // namespace fl

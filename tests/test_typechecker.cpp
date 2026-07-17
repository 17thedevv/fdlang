// =============================================================================
// tests/test_typechecker.cpp
//
// Unit tests for the TypeChecker pass.
//
// Strategy: full pipeline (Lexer → Parser → Resolver → TypeChecker).
// DiagnosticEngine is NOT flushed — errors are inspected via errorCount(),
// so test output is clean with no stderr noise.
//
// Test coverage (12 cases):
//   01. Int variable declaration and use
//   02. Bool variable declaration and use
//   03. Type mismatch on assignment (Int ← Bool)
//   04. Invalid binary operands (Int + Bool)
//   05. if condition must be bool (Int given)
//   06. while condition must be bool (Int given)
//   07. Use before initialization (dec x; print x;)
//   08. Assignment initializes (dec x; x = 5; print x;)
//   09. Int + Int → Int (type inference through binary expr)
//   10. Int < Int → Bool (comparison inference)
//   11. Bool == Bool → Bool (same-type equality)
//   12. Nested: use-before-init inside block
// =============================================================================

#include <cassert>
#include <iostream>
#include "fdlang/FrontEnd/Lexer.h"
#include "fdlang/FrontEnd/Parser.h"
#include "fdlang/MiddleEnd/SymbolTable.h"
#include "fdlang/MiddleEnd/Resolver.h"
#include "fdlang/MiddleEnd/TypeChecker.h"
#include "fdlang/Support/Diagnostic.h"

using namespace fl;

// ── Helper ────────────────────────────────────────────────────────────────────

struct TCResult {
    bool   ok;
    size_t errorCount;
    // TypeChecker is kept alive for typeOf() queries in individual tests.
};

/// Run the full pipeline and return TC results.
/// DiagnosticEngine is NOT flushed (no stderr noise in tests).
/// Resolver errors are included in errorCount, but TC tests use only programs
/// that are expected to resolve cleanly unless otherwise noted.
struct PipelineResult {
    bool             resolveOk;
    bool             typeCheckOk;
    size_t           errorCount;
    // TypeChecker exposed for per-test type queries
    std::unique_ptr<TypeChecker>  tc;
    std::unique_ptr<SymbolTable>  table;
    std::shared_ptr<DiagnosticEngine> diag;
};

PipelineResult runPipeline(const std::string& source) {
    Lexer  lexer(source);
    Parser parser(lexer);
    auto   ast = parser.parse();

    auto diag  = std::make_shared<DiagnosticEngine>();
    auto table = std::make_unique<SymbolTable>();

    Resolver resolver(*table, *diag);
    bool resolveOk = resolver.resolve(ast.get());

    auto tc = std::make_unique<TypeChecker>(*table, *diag);
    bool tcOk = tc->check(ast.get());

    size_t errs = diag->errorCount();

    return {resolveOk, tcOk, errs, std::move(tc), std::move(table), diag};
}

// =============================================================================
// Test 01: Int variable — dec x = 1; print x;
// Expected: ok, no errors
// =============================================================================
void test01_intDeclaration() {
    auto r = runPipeline("dec x = 1; print x;");

    assert(r.resolveOk  && "Test01: should resolve");
    assert(r.typeCheckOk && "Test01: should type-check");
    assert(r.errorCount == 0 && "Test01: no errors");
    assert(r.tc->typeOf(0) == FLType::Int && "Test01: x should be Int");

    std::cout << "[OK] test01_intDeclaration\n";
}

// =============================================================================
// Test 02: Bool variable — dec b = true; print b;
// Expected: ok, b is Bool
// =============================================================================
void test02_boolDeclaration() {
    auto r = runPipeline("dec b = true; print b;");

    assert(r.resolveOk   && "Test02: should resolve");
    assert(r.typeCheckOk && "Test02: should type-check");
    assert(r.errorCount == 0 && "Test02: no errors");
    assert(r.tc->typeOf(0) == FLType::Bool && "Test02: b should be Bool");

    std::cout << "[OK] test02_boolDeclaration\n";
}

// =============================================================================
// Test 03: Mismatched assignment — dec x = 1; x = true;
// Expected: type error
// =============================================================================
void test03_mismatchedAssignment() {
    auto r = runPipeline("dec x = 1; x = true;");

    assert(r.resolveOk    && "Test03: should resolve");
    assert(!r.typeCheckOk && "Test03: type check should fail");
    assert(r.errorCount >= 1 && "Test03: at least 1 error");

    std::cout << "[OK] test03_mismatchedAssignment\n";
}

// =============================================================================
// Test 04: Invalid binary operands — dec x = 1; dec y = true; dec z = x + y;
// Expected: type error (Int + Bool not allowed)
// =============================================================================
void test04_invalidBinaryOperands() {
    auto r = runPipeline("dec x = 1; dec y = true; dec z = x + y;");

    assert(r.resolveOk    && "Test04: should resolve");
    assert(!r.typeCheckOk && "Test04: type check should fail");
    assert(r.errorCount >= 1 && "Test04: at least 1 error");

    std::cout << "[OK] test04_invalidBinaryOperands\n";
}

// =============================================================================
// Test 05: Non-bool if condition — dec x = 1; if (x) {}
// Expected: type error (condition must be Bool)
// =============================================================================
void test05_nonBoolIfCondition() {
    auto r = runPipeline("dec x = 1; if (x) {}");

    assert(r.resolveOk    && "Test05: should resolve");
    assert(!r.typeCheckOk && "Test05: type check should fail");
    assert(r.errorCount >= 1 && "Test05: at least 1 error");

    std::cout << "[OK] test05_nonBoolIfCondition\n";
}

// =============================================================================
// Test 06: Non-bool while condition — dec x = 1; while (x) {}
// Expected: type error (condition must be Bool)
// =============================================================================
void test06_nonBoolWhileCondition() {
    auto r = runPipeline("dec x = 1; while (x) {}");

    assert(r.resolveOk    && "Test06: should resolve");
    assert(!r.typeCheckOk && "Test06: type check should fail");
    assert(r.errorCount >= 1 && "Test06: at least 1 error");

    std::cout << "[OK] test06_nonBoolWhileCondition\n";
}

// =============================================================================
// Test 07: Use before initialization — dec x; print x;
// Expected: type error (x used before init)
// =============================================================================
void test07_useBeforeInitialization() {
    auto r = runPipeline("dec x; print x;");

    assert(r.resolveOk    && "Test07: should resolve");
    assert(!r.typeCheckOk && "Test07: type check should fail");
    assert(r.errorCount >= 1 && "Test07: at least 1 error");

    std::cout << "[OK] test07_useBeforeInitialization\n";
}

// =============================================================================
// Test 08: Assignment initializes — dec x; x = 5; print x;
// Expected: ok (Unknown→Int promoted on first assignment)
// =============================================================================
void test08_assignmentInitializes() {
    auto r = runPipeline("dec x; x = 5; print x;");

    assert(r.resolveOk   && "Test08: should resolve");
    assert(r.typeCheckOk && "Test08: type check should succeed");
    assert(r.errorCount == 0 && "Test08: no errors");
    assert(r.tc->typeOf(0) == FLType::Int && "Test08: x promoted to Int");

    std::cout << "[OK] test08_assignmentInitializes\n";
}

// =============================================================================
// Test 09: Int + Int → Int
// Expected: ok, z is inferred as Int
// =============================================================================
void test09_intPlusIntIsInt() {
    auto r = runPipeline("dec x = 1; dec y = 2; dec z = x + y;");

    assert(r.resolveOk   && "Test09: should resolve");
    assert(r.typeCheckOk && "Test09: should type-check");
    assert(r.errorCount == 0 && "Test09: no errors");
    // z has symbolId=2 (x=0, y=1, z=2)
    assert(r.tc->typeOf(2) == FLType::Int && "Test09: z should be Int");

    std::cout << "[OK] test09_intPlusIntIsInt\n";
}

// =============================================================================
// Test 10: Int < Int → Bool
// Expected: ok, b is inferred as Bool
// =============================================================================
void test10_intComparisonIsBool() {
    auto r = runPipeline("dec b = 1 < 2;");

    assert(r.resolveOk   && "Test10: should resolve");
    assert(r.typeCheckOk && "Test10: should type-check");
    assert(r.errorCount == 0 && "Test10: no errors");
    assert(r.tc->typeOf(0) == FLType::Bool && "Test10: b should be Bool");

    std::cout << "[OK] test10_intComparisonIsBool\n";
}

// =============================================================================
// Test 11: Bool == Bool → Bool
// Expected: ok, result of equality is Bool
// =============================================================================
void test11_boolEqualityIsBool() {
    auto r = runPipeline("dec x = true; dec y = false; dec z = x == y;");

    assert(r.resolveOk   && "Test11: should resolve");
    assert(r.typeCheckOk && "Test11: should type-check");
    assert(r.errorCount == 0 && "Test11: no errors");
    // z has symbolId=2 (x=0, y=1, z=2)
    assert(r.tc->typeOf(2) == FLType::Bool && "Test11: z should be Bool");

    std::cout << "[OK] test11_boolEqualityIsBool\n";
}

// =============================================================================
// Test 12: Use-before-init inside a block
// Expected: type error (inner x not initialized)
// =============================================================================
void test12_useBeforeInitInsideBlock() {
    auto r = runPipeline(R"(
        {
            dec inner;
            print inner;
        }
    )");

    assert(r.resolveOk    && "Test12: should resolve");
    assert(!r.typeCheckOk && "Test12: type check should fail");
    assert(r.errorCount >= 1 && "Test12: at least 1 error");

    std::cout << "[OK] test12_useBeforeInitInsideBlock\n";
}

// =============================================================================
// Main
// =============================================================================

int main() {
    std::cout << "========================================\n";
    std::cout << "  FDLANG TYPE CHECKER TESTS\n";
    std::cout << "========================================\n";

    test01_intDeclaration();
    test02_boolDeclaration();
    test03_mismatchedAssignment();
    test04_invalidBinaryOperands();
    test05_nonBoolIfCondition();
    test06_nonBoolWhileCondition();
    test07_useBeforeInitialization();
    test08_assignmentInitializes();
    test09_intPlusIntIsInt();
    test10_intComparisonIsBool();
    test11_boolEqualityIsBool();
    test12_useBeforeInitInsideBlock();

    std::cout << "========================================\n";
    std::cout << "  ALL 12 TESTS PASSED\n";
    std::cout << "========================================\n";
    return 0;
}

// =============================================================================
// tests/test_resolver.cpp
//
// Unit tests for the Resolver subsystem.
//
// Strategy: use the full pipeline (Lexer → Parser → Resolver) for each test.
// This validates that the Resolver integrates correctly with the Parser output
// and catches bugs that manual AST construction would miss.
//
// Test coverage (12 edge cases):
//   1.  Simple declaration and use
//   2.  Use before declare (error expected)
//   3.  Undeclared identifier (error expected)
//   4.  Double declaration in same scope (error expected)
//   5.  Shadowing in nested scope (should succeed)
//   6.  Variable in inner scope not visible after scope closes (error expected)
//   7.  Assignment to undeclared variable (error expected)
//   8.  print resolves its argument (should succeed)
//   9.  if/else branches each have their own scope
//   10. while loop body scope is isolated
//   11. Declaration with no initializer (dec x;) succeeds
//   12. Re-use of name after inner scope closes (outer still accessible)
// =============================================================================

#include <cassert>
#include <iostream>
#include <sstream>
#include "fdlang/FrontEnd/Lexer.h"
#include "fdlang/FrontEnd/Parser.h"
#include "fdlang/MiddleEnd/SymbolTable.h"
#include "fdlang/MiddleEnd/Resolver.h"
#include "fdlang/Support/Diagnostic.h"

using namespace fl;

// ── Helper ────────────────────────────────────────────────────────────────────

struct ResolveResult {
    bool        ok;
    size_t      errorCount;
    size_t      symbolCount;
    size_t      scopeCount;
};

/// Run the full pipeline on a source string and return resolution results.
/// DiagnosticEngine buffers diagnostics silently; we do NOT call flush(),
/// so error-expected tests produce no stderr noise without any redirect hack.
ResolveResult runResolver(const std::string& source,
                          bool /*suppressErrors*/ = false) {
    Lexer  lexer(source);
    Parser parser(lexer);
    auto   ast = parser.parse();

    DiagnosticEngine diag;
    SymbolTable      table;
    Resolver         resolver(table, diag);
    bool ok = resolver.resolve(ast.get());

    return {ok, diag.errorCount(), table.symbolCount(), table.scopeCount()};
}

// =============================================================================
// Test 1: Simple declaration and use
// Expected: success, 1 symbol, 1 scope (global)
// =============================================================================
void test01_simpleDeclarationAndUse() {
    auto r = runResolver(R"(
        dec x = 10;
        print x;
    )");

    assert(r.ok && "Test01: simple declaration and use should succeed");
    assert(r.symbolCount == 1 && "Test01: exactly 1 symbol (x)");
    std::cout << "[OK] test01_simpleDeclarationAndUse\n";
}

// =============================================================================
// Test 2: Use before declare
// Expected: Resolver error — x not yet in scope when 'print x' runs
// =============================================================================
void test02_useBeforeDeclare() {
    auto r = runResolver(R"(
        print x;
        dec x = 10;
    )", /*suppressErrors=*/true);

    assert(!r.ok && "Test02: use before declare should fail");
    assert(r.errorCount == 1 && "Test02: exactly 1 error");
    std::cout << "[OK] test02_useBeforeDeclare\n";
}

// =============================================================================
// Test 3: Undeclared identifier
// Expected: Resolver error — z never declared anywhere
// =============================================================================
void test03_undeclaredIdentifier() {
    auto r = runResolver(R"(
        print z;
    )", /*suppressErrors=*/true);

    assert(!r.ok && "Test03: undeclared identifier should fail");
    assert(r.errorCount >= 1 && "Test03: at least 1 error");
    std::cout << "[OK] test03_undeclaredIdentifier\n";
}

// =============================================================================
// Test 4: Double declaration in same scope
// Expected: Resolver error — cannot redeclare x in the same scope
// =============================================================================
void test04_doubleDeclarationSameScope() {
    auto r = runResolver(R"(
        dec x = 1;
        dec x = 2;
    )", /*suppressErrors=*/true);

    assert(!r.ok && "Test04: double declaration should fail");
    assert(r.errorCount >= 1 && "Test04: at least 1 error");
    std::cout << "[OK] test04_doubleDeclarationSameScope\n";
}

// =============================================================================
// Test 5: Shadowing in nested scope
// Expected: success — inner 'dec x' is a NEW symbol that shadows outer
// =============================================================================
void test05_shadowingInNestedScope() {
    auto r = runResolver(R"(
        dec x = 1;
        {
            dec x = 2;
            print x;
        }
        print x;
    )");

    assert(r.ok && "Test05: shadowing should succeed");
    // 2 symbols: outer x (symbolId=0) and inner x (symbolId=1)
    assert(r.symbolCount == 2 && "Test05: 2 symbols (outer x and inner x)");
    std::cout << "[OK] test05_shadowingInNestedScope\n";
}

// =============================================================================
// Test 6: Variable in inner scope not visible after scope closes
// Expected: Resolver error — inner x not visible in outer scope
// =============================================================================
void test06_innerScopeVariableInvisibleOutside() {
    auto r = runResolver(R"(
        {
            dec inner = 5;
        }
        print inner;
    )", /*suppressErrors=*/true);

    assert(!r.ok && "Test06: inner scope variable should not be visible outside");
    assert(r.errorCount >= 1 && "Test06: at least 1 error");
    std::cout << "[OK] test06_innerScopeVariableInvisibleOutside\n";
}

// =============================================================================
// Test 7: Assignment to undeclared variable
// Expected: Resolver error — y never declared
// =============================================================================
void test07_assignToUndeclared() {
    auto r = runResolver(R"(
        y = 5;
    )", /*suppressErrors=*/true);

    assert(!r.ok && "Test07: assignment to undeclared should fail");
    assert(r.errorCount >= 1 && "Test07: at least 1 error");
    std::cout << "[OK] test07_assignToUndeclared\n";
}

// =============================================================================
// Test 8: print resolves its argument
// Expected: success — print resolves 'x' correctly, no symbol for 'print' itself
// =============================================================================
void test08_printResolvesArgument() {
    auto r = runResolver(R"(
        dec x = 42;
        print x;
    )");

    assert(r.ok && "Test08: print with valid argument should succeed");
    assert(r.symbolCount == 1 && "Test08: 1 symbol (x), print has no symbol");
    std::cout << "[OK] test08_printResolvesArgument\n";
}

// =============================================================================
// Test 9: if/else branches each have their own scope
// Expected: success — 'a' in then-branch and 'b' in else-branch are isolated
// =============================================================================
void test09_ifElseSeparateScopes() {
    auto r = runResolver(R"(
        dec cond = true;
        if (cond) {
            dec a = 1;
            print a;
        } else {
            dec b = 2;
            print b;
        }
    )");

    assert(r.ok && "Test09: if/else with separate scope variables should succeed");
    // 3 symbols: cond, a, b
    assert(r.symbolCount == 3 && "Test09: 3 symbols (cond, a, b)");
    std::cout << "[OK] test09_ifElseSeparateScopes\n";
}

// =============================================================================
// Test 10: while loop body scope is isolated
// Expected: success, and 'counter' inside loop not visible outside
// =============================================================================
void test10_whileLoopBodyScope() {
    auto r = runResolver(R"(
        dec i = 0;
        while (i < 5) {
            dec loopVar = i;
            i = i + 1;
        }
    )");

    assert(r.ok && "Test10: while loop with body-scoped variable should succeed");
    // 2 symbols: i (global), loopVar (inside body)
    assert(r.symbolCount == 2 && "Test10: 2 symbols (i, loopVar)");
    std::cout << "[OK] test10_whileLoopBodyScope\n";
}

// =============================================================================
// Test 11: Declaration with no initializer (dec x;)
// Expected: success — symbolId is assigned, Resolver does not check initialization
// =============================================================================
void test11_declarationNoInitializer() {
    auto r = runResolver(R"(
        dec x;
    )");

    assert(r.ok && "Test11: declaration with no initializer should succeed in Resolver");
    assert(r.symbolCount == 1 && "Test11: 1 symbol (x)");
    std::cout << "[OK] test11_declarationNoInitializer\n";
}

// =============================================================================
// Test 12: Re-use of name after inner scope closes
// Expected: success — outer x is accessible after the inner block closes
// =============================================================================
void test12_outerNameAccessibleAfterInnerScopeCloses() {
    auto r = runResolver(R"(
        dec x = 10;
        {
            dec x = 20;
        }
        print x;
    )");

    assert(r.ok && "Test12: outer x accessible after inner scope closes");
    // 2 symbols: outer x (id=0) and inner x (id=1)
    // The 'print x' at the end resolves to outer x (id=0)
    assert(r.symbolCount == 2 && "Test12: 2 symbols (outer x, inner x)");
    std::cout << "[OK] test12_outerNameAccessibleAfterInnerScopeCloses\n";
}

// =============================================================================
// Main
// =============================================================================

int main() {
    std::cout << "========================================\n";
    std::cout << "  FDLANG RESOLVER TESTS\n";
    std::cout << "========================================\n";

    test01_simpleDeclarationAndUse();
    test02_useBeforeDeclare();
    test03_undeclaredIdentifier();
    test04_doubleDeclarationSameScope();
    test05_shadowingInNestedScope();
    test06_innerScopeVariableInvisibleOutside();
    test07_assignToUndeclared();
    test08_printResolvesArgument();
    test09_ifElseSeparateScopes();
    test10_whileLoopBodyScope();
    test11_declarationNoInitializer();
    test12_outerNameAccessibleAfterInnerScopeCloses();

    std::cout << "========================================\n";
    std::cout << "  ALL 12 TESTS PASSED\n";
    std::cout << "========================================\n";
    return 0;
}

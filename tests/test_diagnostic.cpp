// =============================================================================
// tests/test_diagnostic.cpp
//
// Unit tests for DiagnosticEngine.
//
// Strategy: test the engine in isolation, without any compiler pass.
// These tests verify the contract that Resolver (and future passes) rely on.
//
// Test coverage (8 cases):
//   1. Fresh engine has zero errors and warnings.
//   2. error() increments errorCount.
//   3. fatal() increments errorCount (Fatal counts as an error).
//   4. warning() increments warningCount, not errorCount.
//   5. note() does not increment either counter.
//   6. hasErrors() reflects errorCount > 0.
//   7. allDiagnostics() returns all recorded diagnostics in order.
//   8. reset() clears all state.
// =============================================================================

#include <cassert>
#include <iostream>
#include "mellis/Support/Diagnostic.h"

using namespace fl;

// Helper: a zero SourceLocation (MVP default — line/col not tracked yet).
static SourceLocation noLoc() { return SourceLocation{}; }

// =============================================================================
// Test 1: Fresh engine is clean
// =============================================================================
void test01_freshEngineIsClean() {
    DiagnosticEngine diag;

    assert(diag.errorCount()   == 0 && "Test01: fresh engine has 0 errors");
    assert(diag.warningCount() == 0 && "Test01: fresh engine has 0 warnings");
    assert(!diag.hasErrors()        && "Test01: fresh engine has no errors");
    assert(diag.allDiagnostics().empty() && "Test01: no diagnostics recorded");

    std::cout << "[OK] test01_freshEngineIsClean\n";
}

// =============================================================================
// Test 2: error() increments errorCount
// =============================================================================
void test02_errorIncrementsCount() {
    DiagnosticEngine diag;
    diag.error(noLoc(), "test error");

    assert(diag.errorCount() == 1 && "Test02: 1 error recorded");
    assert(diag.hasErrors()       && "Test02: hasErrors is true");
    assert(diag.warningCount() == 0 && "Test02: no warnings");

    std::cout << "[OK] test02_errorIncrementsCount\n";
}

// =============================================================================
// Test 3: fatal() also increments errorCount
// =============================================================================
void test03_fatalCountsAsError() {
    DiagnosticEngine diag;
    diag.fatal(noLoc(), "unrecoverable failure");

    assert(diag.errorCount() == 1 && "Test03: Fatal counts as 1 error");
    assert(diag.hasErrors()       && "Test03: hasErrors is true");
    assert(diag.allDiagnostics().back().severity == DiagSeverity::Fatal
           && "Test03: severity is Fatal");

    std::cout << "[OK] test03_fatalCountsAsError\n";
}

// =============================================================================
// Test 4: warning() increments warningCount only
// =============================================================================
void test04_warningDoesNotCountAsError() {
    DiagnosticEngine diag;
    diag.warning(noLoc(), "minor issue");

    assert(diag.warningCount() == 1 && "Test04: 1 warning recorded");
    assert(diag.errorCount()   == 0 && "Test04: no errors");
    assert(!diag.hasErrors()        && "Test04: hasErrors is false");

    std::cout << "[OK] test04_warningDoesNotCountAsError\n";
}

// =============================================================================
// Test 5: note() does not increment any counter
// =============================================================================
void test05_noteDoesNotIncrementCounters() {
    DiagnosticEngine diag;
    diag.note(noLoc(), "supplementary context");

    assert(diag.errorCount()   == 0 && "Test05: no errors");
    assert(diag.warningCount() == 0 && "Test05: no warnings");
    assert(diag.allDiagnostics().size() == 1 && "Test05: 1 diagnostic recorded");

    std::cout << "[OK] test05_noteDoesNotIncrementCounters\n";
}

// =============================================================================
// Test 6: hasErrors reflects errorCount > 0
// =============================================================================
void test06_hasErrorsReflectsCount() {
    DiagnosticEngine diag;
    assert(!diag.hasErrors() && "Test06: no errors initially");

    diag.warning(noLoc(), "w");
    assert(!diag.hasErrors() && "Test06: warning alone doesn't set hasErrors");

    diag.error(noLoc(), "e");
    assert(diag.hasErrors() && "Test06: error sets hasErrors");

    std::cout << "[OK] test06_hasErrorsReflectsCount\n";
}

// =============================================================================
// Test 7: allDiagnostics() preserves insertion order and severity
// =============================================================================
void test07_allDiagnosticsPreservesOrder() {
    DiagnosticEngine diag;
    diag.note   (noLoc(), "note 1");
    diag.warning(noLoc(), "warn 1");
    diag.error  (noLoc(), "err 1");
    diag.fatal  (noLoc(), "fatal 1");

    const auto& all = diag.allDiagnostics();
    assert(all.size() == 4 && "Test07: 4 diagnostics recorded");
    assert(all[0].severity == DiagSeverity::Note    && "Test07: [0] is Note");
    assert(all[1].severity == DiagSeverity::Warning && "Test07: [1] is Warning");
    assert(all[2].severity == DiagSeverity::Error   && "Test07: [2] is Error");
    assert(all[3].severity == DiagSeverity::Fatal   && "Test07: [3] is Fatal");
    assert(all[2].message  == "err 1"               && "Test07: message preserved");

    std::cout << "[OK] test07_allDiagnosticsPreservesOrder\n";
}

// =============================================================================
// Test 8: reset() clears all state
// =============================================================================
void test08_resetClearsState() {
    DiagnosticEngine diag;
    diag.error(noLoc(), "e1");
    diag.warning(noLoc(), "w1");

    diag.reset();

    assert(diag.errorCount()   == 0 && "Test08: errorCount cleared");
    assert(diag.warningCount() == 0 && "Test08: warningCount cleared");
    assert(!diag.hasErrors()        && "Test08: hasErrors false after reset");
    assert(diag.allDiagnostics().empty() && "Test08: diagnostics list cleared");

    std::cout << "[OK] test08_resetClearsState\n";
}

// =============================================================================
// Main
// =============================================================================

int main() {
    std::cout << "========================================\n";
    std::cout << "  MELLIS DIAGNOSTIC ENGINE TESTS\n";
    std::cout << "========================================\n";

    test01_freshEngineIsClean();
    test02_errorIncrementsCount();
    test03_fatalCountsAsError();
    test04_warningDoesNotCountAsError();
    test05_noteDoesNotIncrementCounters();
    test06_hasErrorsReflectsCount();
    test07_allDiagnosticsPreservesOrder();
    test08_resetClearsState();

    std::cout << "========================================\n";
    std::cout << "  ALL 8 TESTS PASSED\n";
    std::cout << "========================================\n";
    return 0;
}

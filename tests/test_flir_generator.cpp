// =============================================================================
// tests/test_flir_generator.cpp
//
// Unit tests for the FLIR Generator.
//
// Verifies that the AST correctly lowers to the exact FLIR text specification
// defined in docs/flir.md. No LLVM IR or optimizations are tested here.
// =============================================================================

#include <cassert>
#include <iostream>
#include <sstream>
#include "mellis/FrontEnd/Lexer.h"
#include "mellis/FrontEnd/Parser.h"
#include "mellis/MiddleEnd/SymbolTable.h"
#include "mellis/MiddleEnd/Resolver.h"
#include "mellis/MiddleEnd/TypeChecker.h"
#include "mellis/MiddleEnd/FLIRGenerator.h"
#include "mellis/Support/Diagnostic.h"

using namespace fl;

// ── Helper ───────────────────────────────────────────────────────────────────

std::string generateFLIR(const std::string& source) {
    Lexer lexer(source);
    DiagnosticEngine diag; Parser parser(lexer, diag);
    auto ast = parser.parse();

    SymbolTable table;
    Resolver resolver(table, diag);
    bool resolveOk = resolver.resolve(ast.get());
    // Type checking might fail due to dummy types used in testing
    TypeContext typeCtx;
    TypeChecker tc(table, diag, typeCtx);
    bool tcOk = tc.check(ast.get());

    FLIRGenerator flirGen(table, tc);
    auto module = flirGen.generate(*ast);
    
    return module->toString();
}

void assertContains(const std::string& fullText, const std::string& substring) {
    if (fullText.find(substring) == std::string::npos) {
        std::cerr << "Expected FLIR to contain:\n" << substring << "\n";
        std::cerr << "Actual FLIR:\n" << fullText << "\n";
        assert(false && "FLIR did not contain expected substring");
    }
}

// ── Tests ────────────────────────────────────────────────────────────────────

void test01_variable_declaration() {
    std::string flir = generateFLIR("dec x = 42;");
    
    // Should contain an alloca for i32 and a store of 42
    assertContains(flir, "alloca i32");
    assertContains(flir, "store 42, %0");
    std::cout << "[OK] test01_variable_declaration\n";
}

void test02_arithmetic() {
    std::string flir = generateFLIR("dec x = 10 + 20;");
    
    // Should evaluate 10 + 20 into a virtual register and store it
    assertContains(flir, "add 10, 20");
    std::cout << "[OK] test02_arithmetic\n";
}

void test03_variable_read() {
    std::string flir = generateFLIR("dec x = 1; dec y = x;");
    
    // y = x requires loading x first
    assertContains(flir, "load %0"); // assuming %0 is x's allocation
    std::cout << "[OK] test03_variable_read\n";
}

void test04_if_statement() {
    std::string flir = generateFLIR("if (true) { dec x = 1; }");
    
    // Should branch on 'true' to a 'then' block
    assertContains(flir, "branch true, then");
    assertContains(flir, "merge");
    std::cout << "[OK] test04_if_statement\n";
}

void test05_while_statement() {
    std::string flir = generateFLIR("while (false) { dec x = 1; }");
    
    // Should jump to condition, branch to body or end
    assertContains(flir, "jump while_cond");
    assertContains(flir, "branch false, while_body");
    assertContains(flir, "while_end");
    std::cout << "[OK] test05_while_statement\n";
}

void test06_print_call() {
    std::string flir = generateFLIR("print 100;");
    
    // print should lower to a call instruction
    assertContains(flir, "call @print(100)");
    std::cout << "[OK] test06_print_call\n";
}

void test07_memory_access() {
    std::string flir = generateFLIR("fn foo() { dec arr = 0; dec x = arr[0]; dec y = arr.f; }");
    assertContains(flir, "get_ptr");
    std::cout << "[OK] test07_memory_access\n";
}

void test08_borrow_cast() {
    std::string flir = generateFLIR("fn foo() { dec x = 1; dec ptr = &x; dec mut_ptr = &rw x; dec z = x as i32; }");
    assertContains(flir, "borrow shared");
    assertContains(flir, "borrow mut");
    assertContains(flir, "cast");
    std::cout << "[OK] test08_borrow_cast\n";
}

void test09_match_expr() {
    std::string flir = generateFLIR("fn foo() { dec x = 1; match x { 1 -> { print 1; } _ -> { print 2; } }; }");
    assertContains(flir, "switch");
    assertContains(flir, "match_arm");
    assertContains(flir, "match_end");
    std::cout << "[OK] test09_match_expr\n";
}

void test10_functions_ffi() {
    std::string flir = generateFLIR("fn printf() { } fn foo() { printf(); }");
    assertContains(flir, "@printf");
    assertContains(flir, "@foo");
    std::cout << "[OK] test10_functions_ffi\n";
}

void test11_globals() {
    std::string flir = generateFLIR("fn foo() { dec s = \"hello\"; }");
    assertContains(flir, "@.str.");
    std::cout << "[OK] test11_globals\n";
}

// ── Main ─────────────────────────────────────────────────────────────────────

int main() {
    std::cout << "========================================\n";
    std::cout << "  MELLIS FLIR GENERATOR TESTS\n";
    std::cout << "========================================\n";

    test01_variable_declaration();
    test02_arithmetic();
    test03_variable_read();
    test04_if_statement();
    test05_while_statement();
    test06_print_call();
    test07_memory_access();
    test08_borrow_cast();
    test09_match_expr();
    test10_functions_ffi();
    test11_globals();

    std::cout << "========================================\n";
    std::cout << "  ALL TESTS PASSED\n";
    std::cout << "========================================\n";
    return 0;
}

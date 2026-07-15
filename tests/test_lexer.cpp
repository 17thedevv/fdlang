#include <cassert>
#include <iostream>
#include "fdlang/Frontend/Lexer.h"

using namespace fl;

void testVariableDeclaration() {
    Lexer lexer("dec a = 5; fn myFunc() -> int { return a; }");
    
    Token t1 = lexer.nextToken();
    assert(t1.type == TokenType::KW_DEC && "Expected KW_DEC");
    assert(t1.text == "dec" && "Expected text 'dec'");
    
    Token t2 = lexer.nextToken();
    assert(t2.type == TokenType::IDENTIFIER && "Expected IDENTIFIER");
    assert(t2.text == "a" && "Expected text 'a'");
    
    Token t3 = lexer.nextToken();
    assert(t3.type == TokenType::EQUAL && "Expected EQUAL");
    
    Token t4 = lexer.nextToken();
    assert(t4.type == TokenType::NUMBER && "Expected NUMBER");
    assert(t4.text == "5" && "Expected text '5'");
    
    Token t5 = lexer.nextToken();
    assert(t5.type == TokenType::SEMI && "Expected SEMI");
    
    Token t6 = lexer.nextToken();
    assert(t6.type == TokenType::END_OF_FILE && "Expected EOF");

    std::cout << "[OK] testVariableDeclaration passed!\n";
}

int main() {
    std::cout << "--- RUNNING FDLANG TESTS ---\n";
    
    testVariableDeclaration();
    // Sau này bạn có thể viết thêm các hàm test khác và gọi ở đây
    // testMathOperators();
    // testKeywords();
    
    std::cout << "--- ALL TESTS PASSED SUCCESSFULLY! ---\n";
    return 0;
}
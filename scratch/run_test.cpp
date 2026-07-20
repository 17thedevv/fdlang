#include <iostream>
#include "mellis/FrontEnd/Lexer.h"
#include "mellis/FrontEnd/Parser.h"
#include "mellis/FrontEnd/MacroExpander.h"
#include "mellis/FrontEnd/MacroValidator.h"
#include "mellis/FrontEnd/MacroResolver.h"
#include "mellis/FrontEnd/ImportResolver.h"
#include "mellis/Basic/Diagnostics.h"

using namespace fl;
int main() {
    Diagnostics diag;
    Lexer lexer("macro run_all(@stmts: expr...) { @stmts } fn main() { run_all!(a(), b()); }", diag);
    Parser parser(lexer, diag);
    auto ast = parser.parse();
    if (diag.hasErrors()) {
        std::cerr << "PARSER ERRORS:\n";
        diag.printErrors();
        return 1;
    }
    
    MacroRegistry registry;
    MacroResolver resolver(diag, registry);
    ast->accept(resolver);
    if (diag.hasErrors()) {
        std::cerr << "RESOLVER ERRORS:\n";
        diag.printErrors();
        return 1;
    }
    
    MacroValidator validator(diag, registry);
    ast->accept(validator);
    if (diag.hasErrors()) {
        std::cerr << "VALIDATOR ERRORS:\n";
        diag.printErrors();
        return 1;
    }

    MacroExpander expander(diag, registry);
    ast = std::unique_ptr<ProgramNode>(static_cast<ProgramNode*>(expander.transformNode(std::move(ast)).release()));
    if (diag.hasErrors()) {
        std::cerr << "EXPANDER ERRORS:\n";
        diag.printErrors();
        return 1;
    }

    std::cerr << "OK\n";
    return 0;
}

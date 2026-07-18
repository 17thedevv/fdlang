import re
with open('tests/test_executable_generator.cpp', 'r', encoding='utf-8') as f:
    code = f.read()

new_setup = '''    DiagnosticEngine diag;
    SymbolTable table;
    Resolver resolver(table, diag);
    resolver.resolve(ast.get());

    TypeContext ctx;
    TypeChecker tc(table, diag, ctx);
    tc.check(ast.get());

    MatchAnalyzer matchAnalyzer(table, diag);
    matchAnalyzer.analyze(ast.get());

    FLIRGenerator flirGen(table, tc);
    auto* prog = dynamic_cast<ProgramNode*>(ast.get());
    auto flirModule = flirGen.generate(*prog);

    BorrowChecker bc(flirModule.get(), diag);
    bc.check();

    llvm::LLVMContext llvmCtx;
    llvm::Module llvmModule(" test_module\, llvmCtx);
 LLVMIRGenerator llvmGen(llvmCtx, llvmModule, table);
 bool ok = llvmGen.generate(flirModule.get());
 if (!ok) {
 std::cerr << \LLVM generation failed\\n\;
 std::exit(1);
 }

 LLDLinker linker(diag);'''

code = re.sub(r'DiagnosticEngine diag;\s+SymbolTable table;.*?(?=ExecutableGenerator exeGen)', new_setup.strip() + '\n ', code, flags=re.DOTALL)

if 'MatchAnalyzer.h' not in code:
 code = code.replace('#include \fdlang/MiddleEnd/TypeChecker.h\', '#include \fdlang/MiddleEnd/TypeChecker.h\\\n#include \fdlang/MiddleEnd/MatchAnalyzer.h\\\n#include \fdlang/MiddleEnd/BorrowChecker.h\\\n#include \fdlang/Support/LLDLinker.h\')

with open('tests/test_executable_generator.cpp', 'w', encoding='utf-8') as f:
 f.write(code)

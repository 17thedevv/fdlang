#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include "fdlang/FrontEnd/Lexer.h"
#include "fdlang/FrontEnd/Parser.h"
#include "fdlang/MiddleEnd/SymbolTable.h"
#include "fdlang/MiddleEnd/Resolver.h"
#include "fdlang/MiddleEnd/TypeChecker.h"
#include "fdlang/MiddleEnd/FLIRGenerator.h"
#include "fdlang/BackEnd/LLVMIRGenerator.h"
#include "fdlang/BackEnd/ExecutableGenerator.h"
#include "fdlang/Support/ClangLinker.h"
#include "fdlang/Support/Diagnostic.h"

using namespace fl;

std::string readFile(const std::string& filepath, DiagnosticEngine& diag) {
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        diag.error(SourceLocation{}, "Khong the mo file '" + filepath + "'");
        return "";
    }
    std::streamsize size = file.tellg();
    std::string buffer(size, '\0');
    file.seekg(0, std::ios::beg);
    if (file.read(buffer.data(), size)) return buffer;
    diag.error(SourceLocation{}, "Khong the doc noi dung file '" + filepath + "'");
    return "";
}

int main(int argc, char* argv[]) {
    DiagnosticEngine diag;

    if (argc < 2) {
        diag.error(SourceLocation{}, "Cach su dung: fdlang <file_path> [-v|--verbose]");
        diag.flush();
        return 64;
    }

    std::string filepath;
    bool verbose = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-v" || arg == "--verbose") {
            verbose = true;
        } else if (filepath.empty()) {
            filepath = arg;
        } else {
            diag.error(SourceLocation{}, "Qua nhieu tham so, khong ho tro: " + arg);
            diag.flush();
            return 64;
        }
    }

    if (filepath.empty()) {
        diag.error(SourceLocation{}, "Thieu <file_path>");
        diag.flush();
        return 64;
    }

    std::string sourceCode = readFile(filepath, diag);
    if (sourceCode.empty() && diag.hasErrors()) {
        diag.flush();
        return 74;
    }

    std::cout << "[FDLANG COMPILER - RESOLVER PHASE]\n";
    std::cout << "Compiling: " << filepath << "\n";
    std::cout << "-----------------------------------\n";

    // ── Phase 1: Lexer ────────────────────────────────────────────────────────
    Lexer lexer(sourceCode);

    // ── Phase 2: Parser ───────────────────────────────────────────────────────
    std::cout << "[1] Phan tich cu phap (Parser)...\n";
    Parser parser(lexer);
    auto ast = parser.parse();
    std::cout << "[2] Parse thanh cong! ("
              << ast->statements.size() << " cau lenh)\n";

    // ── Phase 3: Resolver ─────────────────────────────────────────────────────
    // Resolver is the first MiddleEnd pass.
    // It annotates every IdentifierExpr, VarDeclStmt, and AssignStmtNode
    // with a SymbolID that connects the usage to its declaration.
    //
    // TODO(CompilerContext): diag, symbolTable, and future SourceManager /
    // StringInterner will be owned by a shared CompilerContext passed here.
    std::cout << "[3] Giai quyet ten (Resolver)...\n";
    SymbolTable symbolTable;
    Resolver resolver(symbolTable, diag);
    bool resolveOk = resolver.resolve(ast.get());

    if (!resolveOk) {
        diag.error(SourceLocation{}, "Resolver that bai voi " + std::to_string(diag.errorCount()) + " loi.");
        diag.flush();
        return 65;
    }

    std::cout << "[4] Resolver thanh cong! ("
              << symbolTable.symbolCount() << " symbols, "
              << symbolTable.scopeCount()  << " scopes)\n";

    // ── Phase 4: TypeChecker ──────────────────────────────────────────────────
    // TypeChecker infers and verifies types for all expressions and statements.
    // It populates ExprNode::inferredType on every expression for FLIR use.
    std::cout << "[5] Kiem tra kieu du lieu (TypeChecker)...\n";
    TypeChecker typeChecker(symbolTable, diag);
    bool tcOk = typeChecker.check(ast.get());

    if (!tcOk) {
        diag.error(SourceLocation{}, "TypeChecker that bai voi " + std::to_string(diag.errorCount()) + " loi.");
        diag.flush();
        return 65;
    }

    std::cout << "[6] TypeChecker thanh cong!\n";

    // ── Phase 5: FLIR Generator ───────────────────────────────────────────────
    // FLIRGenerator takes the typed AST and lowers it to FLIR.
    std::cout << "[7] Tao ma trung gian (FLIRGenerator)...\n";
    FLIRGenerator flirGen(symbolTable, typeChecker);
    auto flirModule = flirGen.generate(ast.get());

    std::cout << "[8] Tao LLVM IR (LLVMIRGenerator)...\n";
    llvm::LLVMContext llvmContext;
    llvm::Module llvmModule(filepath, llvmContext);
    LLVMIRGenerator llvmGen(llvmContext, llvmModule);
    bool llvmOk = llvmGen.generate(flirModule.get());

    if (!llvmOk) {
        diag.error(SourceLocation{}, "LLVM Module verification failed.");
        diag.flush();
        return 65;
    }

    std::cout << "-----------------------------------\n";
    std::cout.flush();
    // (Optional: can comment out module dump to keep output clean in final build)
    // llvmModule.print(llvm::outs(), nullptr);
    // llvm::outs().flush();

    std::cout << "[9] Tao thuc thi (ExecutableGenerator)...\n";
    std::cout.flush();
    ClangLinker linker(diag, verbose);
    ExecutableGenerator exeGen(diag, linker);
    
    // Derive output executable name from source file name
    std::string outExe = filepath;
    size_t dotPos = outExe.find_last_of('.');
    if (dotPos != std::string::npos) {
        outExe = outExe.substr(0, dotPos);
    }
    outExe += ".exe";

    bool exeOk = exeGen.generateExecutable(&llvmModule, outExe);
    if (!exeOk) {
        diag.error(SourceLocation{}, "Loi khi tao file thuc thi.");
        diag.flush();
        return 66;
    }

    std::cout << "-----------------------------------\n";
    std::cout << "Build hoan tat: " << outExe << "\n";

    return 0;
}
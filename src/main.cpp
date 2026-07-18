#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <cassert>
#include "mellis/Core/SourceLocation.h"
#include "mellis/FrontEnd/Lexer.h"
#include "mellis/FrontEnd/Parser.h"
#include "mellis/MiddleEnd/SymbolTable.h"
#include "mellis/MiddleEnd/Resolver.h"
#include "mellis/MiddleEnd/TypeChecker.h"
#include "mellis/MiddleEnd/MatchAnalyzer.h"
#include "mellis/MiddleEnd/FLIRGenerator.h"
#include "mellis/MiddleEnd/MonomorphizationEngine.h"
#include "mellis/MiddleEnd/BorrowChecker.h"
#include "mellis/BackEnd/LLVMIRGenerator.h"
#include "mellis/BackEnd/ExecutableGenerator.h"
#include "mellis/Support/LLDLinker.h"
#include "mellis/Support/Diagnostic.h"

#include "mellis/Core/SourceManager.h"

using namespace fl;


int main(int argc, char* argv[]) {
    std::cout << "[MELLIS COMPILER - RESOLVER PHASE]" << std::endl;
    DiagnosticEngine diag;
    if (argc < 2) {
        std::cerr << "mellis: error: Cach su dung: mellis <file_path> [-v|--verbose]\n";
        return 1;
    }

    std::string filepath = argv[1];
    bool verbose = true;
    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-v" || arg == "--verbose") {
            verbose = true;
        }
    }

    SourceManager sourceManager(diag);
    FileID mainFileId = sourceManager.loadFile(filepath);
    if (mainFileId == SourceManager::kInvalidFileID) {
        diag.flush();
        return 65; // data format error
    }

    std::cout << "Compiling: " << filepath << std::endl;
    std::cout << "-----------------------------------" << std::endl;

    // ── Phase 1 & 2: Lexer and Parser ─────────────────────────────────────────
    std::cout << "[1] Phan tich cu phap (Parser)..." << std::endl;
    std::string_view sourceCode = sourceManager.getSource(mainFileId);
    Lexer lexer(sourceCode);
    Parser parser(lexer, diag, &sourceManager, mainFileId);
    auto ast = parser.parse();

    if (diag.errorCount() > 0 || !ast) {
        diag.error(SourceLocation::invalid(), "Parsing that bai.");
        diag.flush();
        return 65;
    }
    std::cout << "[2] Parse thanh cong! ("
              << ast->items.size() << " cau lenh)" << std::endl;

    // ── Phase 3: Resolver ─────────────────────────────────────────────────────
    std::cout << "[3] Giai quyet ten (Resolver)..." << std::endl;
    SymbolTable symbolTable;
    Resolver resolver(symbolTable, diag);
    bool resolveOk = resolver.resolve(ast.get());

    if (!resolveOk) {
        diag.error(SourceLocation::invalid(), "Resolver that bai, nhung tiep tuc de test FLIR...");
        diag.flush();
        // return 65;
    }

    std::cout << "[4] Resolver thanh cong!" << std::endl;

    // ── Phase 4: TypeChecker ──────────────────────────────────────────────────
    std::cout << "[5] Kiem tra kieu du lieu (TypeChecker)..." << std::endl;
    TypeContext typeContext;
    TypeChecker typeChecker(symbolTable, diag, typeContext);
    MonomorphizationEngine monoEngine(symbolTable, resolver, typeChecker);
    typeChecker.setMonomorphizationEngine(&monoEngine);
    bool tcOk = typeChecker.check(ast.get());

    if (!tcOk) {
        diag.error(SourceLocation::invalid(), "TypeChecker that bai, nhung tiep tuc de test FLIR...");
        diag.flush();
        // return 65;
    }
    std::cout << "[6] Type Checker thanh cong!" << std::endl;

    auto* prog = dynamic_cast<ProgramNode*>(ast.get());
    assert(prog);

    // -- Phase 4.5: Inject Specialized Functions --
    std::cout << "[6.05] Tiem cac ham chuyen biet hoa vao AST..." << std::endl;
    auto specializedFns = monoEngine.takeSpecializedFunctions();
    for (auto& fn : specializedFns) {
        prog->items.push_back(std::move(fn));
    }

    // ── Phase 5: Match Analyzer ──────────────────────────────────────────────
    std::cout << "[6.1] Phan tich Pattern Matching (MatchAnalyzer)..." << std::endl;
    MatchAnalyzer matchAnalyzer(symbolTable, diag);
    bool matchOk = matchAnalyzer.analyze(ast.get());
    if (!matchOk) {
        diag.error(SourceLocation::invalid(), "MatchAnalyzer that bai, nhung tiep tuc de test FLIR...");
        diag.flush();
        // return 65;
    }
    std::cout << "[6.2] MatchAnalyzer thanh cong!" << std::endl;

    // ── Phase 6: FLIR Generation ──────────────────────────────────────────────
    std::cout << "[7] Sinh FLIR (FLIRGenerator)..." << std::endl;
    FLIRGenerator flirGen(symbolTable, typeChecker);
    auto flirModule = flirGen.generate(*prog);
    if (!flirModule) {
        diag.error(SourceLocation::invalid(), "Khong the sinh FLIR.");
        diag.flush();
        return 65;
    }
    std::cout << "[8] Sinh FLIR thanh cong!\n";
    if (verbose) {
        std::cout << "\n=== FLIR ===\n";
        std::cout << flirModule->toString() << "\n";
    }

    // ── Phase 7: Borrow Checker ──────────────────────────────────────────────
    std::cout << "[8.1] Kiem tra muon tham chieu (BorrowChecker)..." << std::endl;
    BorrowChecker borrowChecker(flirModule.get(), diag);
    bool borrowOk = borrowChecker.check();
    if (!borrowOk) {
        diag.error(SourceLocation::invalid(), "BorrowChecker that bai.");
        diag.flush();
        return 65;
    }
    std::cout << "[8.2] BorrowChecker thanh cong!" << std::endl;

    // ── Phase 8: LLVM IR Generation ──────────────────────────────────────────
    llvm::LLVMContext llvmContext;
    llvm::Module llvmModule(filepath, llvmContext);
    
    LLVMIRGenerator llvmGen(llvmContext, llvmModule, symbolTable);
    bool llvmOk = llvmGen.generate(flirModule.get());
    
    if (!llvmOk) {
        diag.error(SourceLocation::invalid(), "Loi trong qua trinh sinh LLVM IR.");
        diag.flush();
        return 65;
    }
    
    if (verbose) {
        std::cout << "\n=== LLVM IR ===\n";
        llvmModule.print(llvm::outs(), nullptr);
        llvm::outs().flush();
    }

    // ── Phase 9: LLD Linker / Executable Generation ──────────────────────────
    std::cout << "[9] Tao thuc thi (ExecutableGenerator)..." << std::endl;
    LLDLinker linker(diag);
    ExecutableGenerator exeGen(diag, linker);
    
    std::string outName = filepath;
    size_t lastDot = outName.find_last_of('.');
    if (lastDot != std::string::npos) {
        outName = outName.substr(0, lastDot);
    }
    outName += ".exe";

    bool exeOk = exeGen.generateExecutable(&llvmModule, outName);
    if (!exeOk) {
        diag.error(SourceLocation::invalid(), "Tao file thuc thi that bai.");
        diag.flush();
        return 65;
    }

    std::cout << "[10] Thanh cong! File dau ra: " << outName << std::endl;

    return 0;
}

#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include "fdlang/FrontEnd/Lexer.h"
#include "fdlang/FrontEnd/Parser.h"
#include "fdlang/MiddleEnd/SymbolTable.h"
#include "fdlang/MiddleEnd/Resolver.h"
#include "fdlang/Support/Diagnostic.h"

using namespace fl;

std::string readFile(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "Loi: Khong the mo file '" << filepath << "'\n";
        std::exit(74);
    }
    std::streamsize size = file.tellg();
    std::string buffer(size, '\0');
    file.seekg(0, std::ios::beg);
    if (file.read(buffer.data(), size)) return buffer;
    std::cerr << "Loi: Khong the doc noi dung file '" << filepath << "'\n";
    std::exit(74);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Cach su dung: fdlang <file_path>\n";
        return 64;
    }

    std::string filepath = argv[1];
    std::string sourceCode = readFile(filepath);

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
    DiagnosticEngine diag;
    SymbolTable symbolTable;
    Resolver resolver(symbolTable, diag);
    bool resolveOk = resolver.resolve(ast.get());

    if (!resolveOk) {
        diag.flush();
        std::cerr << "[!] Resolver that bai voi "
                  << diag.errorCount() << " loi.\n";
        return 65;
    }

    std::cout << "[4] Resolver thanh cong! ("
              << symbolTable.symbolCount() << " symbols, "
              << symbolTable.scopeCount()  << " scopes)\n";
    std::cout << "-----------------------------------\n";
    std::cout << "Build hoan tat.\n";

    // ── Phase 4+: TypeChecker, BorrowChecker, IRGenerator (future) ────────────
    // Each future pass receives:
    //   - ast         (AST with symbolId annotations)
    //   - symbolTable (fully populated registry)
    // and adds its own side tables (TypeTable, BorrowTable, etc.)

    return 0;
}
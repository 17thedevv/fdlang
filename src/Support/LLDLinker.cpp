// =============================================================================
// mellis/Support/LLDLinker.cpp
//
// Implementation of LLDLinker.
// =============================================================================

#include "mellis/Support/LLDLinker.h"
#include <cstdlib>
#include <iostream>

namespace fl {

LLDLinker::LLDLinker(DiagnosticEngine& diag, bool verbose) 
    : diag_(diag), verbose_(verbose) {}

bool LLDLinker::link(const std::string& objFile,
                       const std::string& outFile,
                       const std::vector<std::string>& libs) {
    // Determine the lld-link path. We'll assume the user has it in PATH or it's accessible.
    // Use D:\Programs\LLVM-DEV\bin\lld-link.exe as a hardcoded fallback if needed, or rely on system PATH.
    // For now, we rely on lld-link.
    std::string linkerPath = "lld-link";
    
    // Check if lld-link exists by trying to call it
    int checkResult = std::system("lld-link --version > NUL 2>&1");
    if (checkResult != 0) {
        // Fallback to absolute path on the user's specific setup for LLVM-DEV
        linkerPath = "\"D:\\Programs\\LLVM-DEV\\bin\\lld-link.exe\"";
    }

    std::string command = linkerPath + " /OUT:" + outFile + " " + objFile;
    
    for (const auto& lib : libs) {
        command += " " + lib;
    }

    // Default library linkage for Windows (libcmt provides C runtime)
    command += " /DEFAULTLIB:libcmt";
    command += " /DEFAULTLIB:libucrt";
    command += " /DEFAULTLIB:legacy_stdio_definitions";

    if (!verbose_) {
        std::cout << "Running command: " << command << "\n";
        std::cout.flush();
    } else {
        std::cout << "Running command: " << command << "\n";
        std::cout.flush();
    }

    int result = std::system(command.c_str());
    if (result != 0) {
        diag_.error(SourceLocation{}, "Trinh lien ket (lld-link) that bai khi tao " + outFile + 
                    ". Kiem tra xem ban da cai dat LLVM chua.");
        return false;
    }

    return true;
}

} // namespace fl

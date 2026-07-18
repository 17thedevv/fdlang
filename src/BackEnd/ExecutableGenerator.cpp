// =============================================================================
// mellis/BackEnd/ExecutableGenerator.cpp
// =============================================================================

#include "mellis/BackEnd/ExecutableGenerator.h"
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/Support/CodeGen.h>
#include <system_error>
#include <iostream>

namespace fl {

ExecutableGenerator::ExecutableGenerator(DiagnosticEngine& diag, Linker& linker)
    : diag_(diag), linker_(linker) {}

bool ExecutableGenerator::generateExecutable(llvm::Module* llvmModule, const std::string& outputPath) {
    // 1. Initialize Native Target
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmParser();
    llvm::InitializeNativeTargetAsmPrinter();

    auto targetTriple = llvm::sys::getDefaultTargetTriple();
    llvmModule->setTargetTriple(targetTriple);

    std::string error;
    auto target = llvm::TargetRegistry::lookupTarget(targetTriple, error);

    if (!target) {
        diag_.error(SourceLocation{}, "Khong the tim thay Target cho LLVM: " + error);
        return false;
    }

    auto cpu = "generic";
    auto features = "";

    llvm::TargetOptions opt;
    auto rm = std::optional<llvm::Reloc::Model>();
    auto targetMachine = target->createTargetMachine(targetTriple, cpu, features, opt, rm);

    llvmModule->setDataLayout(targetMachine->createDataLayout());

    // 2. Emit Object File
    std::string objPath = outputPath + ".obj";
    std::error_code ec;
    llvm::raw_fd_ostream dest(objPath, ec, llvm::sys::fs::OF_None);

    if (ec) {
        diag_.error(SourceLocation{}, "Khong the mo file de ghi: " + ec.message());
        return false;
    }

    llvm::legacy::PassManager pass;
    auto fileType = llvm::CodeGenFileType::ObjectFile;

    if (targetMachine->addPassesToEmitFile(pass, dest, nullptr, fileType)) {
        diag_.error(SourceLocation{}, "TargetMachine khong the emit kieu file nay.");
        return false;
    }

    pass.run(*llvmModule);
    dest.flush();

    // 3. Link Object File into Executable
    // For MVP, we hardcode the path to where CMake builds it.
    std::vector<std::string> libs = { "build\\Release\\mellis_rt.lib" };
    return linker_.link(objPath, outputPath, libs);
}

} // namespace fl

// =============================================================================
// mellis/MLib/MLibGenerator.h
//
// Generates the .mlib binary file containing object code and metadata.
// =============================================================================

#pragma once

#include "mellis/Support/Diagnostic.h"
#include "mellis/MiddleEnd/SymbolTable.h"
#include "mellis/Core/FLType.h"
#include "mellis/FrontEnd/MacroRegistry.h"
#include <llvm/IR/Module.h>
#include <string>

namespace fl {

class MLibGenerator {
public:
    MLibGenerator(DiagnosticEngine& diag, SymbolTable& symTab, TypeContext& typeCtx, MacroRegistry& macroReg);

    /// Generate a .mlib file from the LLVM module and SymbolTable metadata.
    ///
    /// @param llvmModule The generated LLVM module.
    /// @param outputPath The desired path to the final .mlib file.
    /// @return true if generation succeeded.
    bool generate(llvm::Module* llvmModule, const std::string& outputPath);

private:
    DiagnosticEngine& diag_;
    SymbolTable& symTab_;
    TypeContext& typeCtx_;
    MacroRegistry& macroReg_;
};

} // namespace fl

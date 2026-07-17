// =============================================================================
// fdlang/BackEnd/ExecutableGenerator.h
//
// ExecutableGenerator — Final lowering pass emitting a native executable.
// =============================================================================

#pragma once

#include "fdlang/Support/Linker.h"
#include "fdlang/Support/Diagnostic.h"
#include <llvm/IR/Module.h>
#include <string>

namespace fl {

class ExecutableGenerator {
public:
    ExecutableGenerator(DiagnosticEngine& diag, Linker& linker);

    /// Generate a native executable from the LLVM module.
    ///
    /// @param llvmModule The generated LLVM module.
    /// @param outputPath The desired path to the final executable (e.g. "ex.exe").
    /// @return true if compilation and linking succeeded.
    bool generateExecutable(llvm::Module* llvmModule, const std::string& outputPath);

private:
    DiagnosticEngine& diag_;
    Linker& linker_;
};

} // namespace fl

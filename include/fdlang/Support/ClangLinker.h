// =============================================================================
// fdlang/Support/ClangLinker.h
//
// Clang-based linker implementation. Uses clang as the compiler driver
// to automatically discover the Windows SDK and invoke lld-link.
// =============================================================================

#ifndef FDLANG_CLANG_LINKER_H
#define FDLANG_CLANG_LINKER_H

#include "fdlang/Support/Linker.h"
#include "fdlang/Support/Diagnostic.h"

namespace fl {

class ClangLinker : public Linker {
public:
    explicit ClangLinker(DiagnosticEngine& diag, bool verbose = false);

    bool link(const std::string& objFile,
              const std::string& outFile,
              const std::vector<std::string>& libs) override;

private:
    DiagnosticEngine& diag_;
    bool verbose_;
};

} // namespace fl

#endif // FDLANG_CLANG_LINKER_H

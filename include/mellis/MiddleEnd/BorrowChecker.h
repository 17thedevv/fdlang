#pragma once

#include <vector>
#include <unordered_map>
#include "mellis/MiddleEnd/FLIR.h"
#include "mellis/MiddleEnd/Place.h"
#include "mellis/Support/Diagnostic.h"

namespace fl {

struct Loan {
    size_t id;
    Place place;
    bool isMutable;
    SourceLocation loc; // Where the borrow occurred
};

class BorrowChecker {
    const flir::Module* module_;
    DiagnosticEngine& diag_;
    
    // Maps virtual registers (LocalId) to the Place they represent.
    // E.g. %1 = get_ptr %arr, 0 => placeMap_["%1"] = Place(arr, [Index(0)])
    std::unordered_map<std::string, Place> placeMap_;
    
    // Active loans in the current flow state
    std::vector<std::vector<Loan>> scopeStack_;
    size_t nextLoanId_ = 0;

public:
    BorrowChecker(const flir::Module* module, DiagnosticEngine& diag)
        : module_(module), diag_(diag) {}

    bool check();

private:
    void checkFunction(const flir::Function& func);
    void checkBlock(const flir::BasicBlock& block);
    void checkInstruction(const flir::Instruction& inst);

    void issueLoan(const Place& place, bool isMut, SourceLocation loc);
    void checkAccess(const Place& place, bool isMut, SourceLocation loc);
    
    Place resolvePlace(const flir::Operand& op) const;
};

} // namespace fl

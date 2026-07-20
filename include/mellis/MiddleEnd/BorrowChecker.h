#pragma once

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include "mellis/MiddleEnd/MVIR.h"
#include "mellis/MiddleEnd/Place.h"
#include "mellis/Support/Diagnostic.h"

namespace fl {

struct Loan {
    size_t id;
    Place place;
    bool isMutable;
    std::string referenceId; // Tên của biến giữ reference này (ví dụ: %1)
    SourceLocation loc;
};

struct LivenessInfo {
    // Map từ Tên Biến (Reference) sang tập các Instruction IDs (hoặc pointers) mà biến đó còn sống
    std::unordered_map<std::string, std::unordered_set<const mvir::Instruction*>> liveInstructions;
};

struct DataflowState {
    std::vector<Loan> activeLoans;
    std::unordered_map<std::string, Place> placeMap;
};

class BorrowChecker {
    const mvir::Module* module_;
    DiagnosticEngine& diag_;
    size_t nextLoanId_ = 0;

public:
    BorrowChecker(const mvir::Module* module, DiagnosticEngine& diag)
        : module_(module), diag_(diag) {}

    bool check();

private:
    void checkFunction(const mvir::Function& func);
    
    // Pass 1: Liveness Analysis
    LivenessInfo computeLiveness(const mvir::Function& func);
    
    // Pass 2: NLL Borrow Checking
    void checkDataflow(const mvir::Function& func, const mvir::BasicBlock& block, DataflowState state, const LivenessInfo& liveness, std::unordered_set<const mvir::BasicBlock*>& visited);

    void checkInstruction(const mvir::Instruction& inst, DataflowState& state, const LivenessInfo& liveness);

    void issueLoan(const Place& place, bool isMut, const std::string& refId, SourceLocation loc, DataflowState& state);
    void checkAccess(const Place& place, bool isMut, SourceLocation loc, const DataflowState& state);
    
    Place resolvePlace(const mvir::Operand& op, const DataflowState& state) const;
};

} // namespace fl

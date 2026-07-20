#include "mellis/MiddleEnd/BorrowChecker.h"
#include <iostream>
#include <algorithm>

namespace fl {

// Helper method to find block by label
const mvir::BasicBlock* findBlock(const mvir::Function& func, const mvir::LabelId& label) {
    for (const auto& block : func.blocks) {
        if (block->label == label) return block.get();
    }
    return nullptr;
}

bool BorrowChecker::check() {
    bool ok = true;
    for (const auto& func : module_->functions) {
        nextLoanId_ = 0;
        checkFunction(*func);
        if (diag_.hasErrors()) ok = false;
    }
    return ok;
}

// ============================================================================
// ============================================================================
// Pass 2: NLL Borrow Checking (Forward Dataflow)
// ============================================================================
void BorrowChecker::checkFunction(const mvir::Function& func) {
    if (func.blocks.empty()) return;
    LivenessInfo liveness = LivenessAnalyzer::computeLiveness(func);
    DataflowState initialState;
    std::unordered_set<const mvir::BasicBlock*> visited;
    checkDataflow(func, *func.blocks.front(), initialState, liveness, visited);
}

void BorrowChecker::checkDataflow(const mvir::Function& func, const mvir::BasicBlock& block, DataflowState state, const LivenessInfo& liveness, std::unordered_set<const mvir::BasicBlock*>& visited) {
    if (visited.count(&block)) return;
    visited.insert(&block);

    for (const auto& instPtr : block.instructions) {
        const mvir::Instruction& inst = *instPtr;
        checkInstruction(inst, state, liveness);
        
        state.activeLoans.erase(
            std::remove_if(state.activeLoans.begin(), state.activeLoans.end(),
                [&](const Loan& loan) {
                    auto it = liveness.liveInstructions.find(loan.referenceId);
                    if (it != liveness.liveInstructions.end()) {
                        return it->second.find(&inst) == it->second.end();
                    }
                    return true; 
                }),
            state.activeLoans.end()
        );
    }
    
    // NLL Kill sau terminator
    state.activeLoans.erase(
        std::remove_if(state.activeLoans.begin(), state.activeLoans.end(),
            [&](const Loan& loan) { return false; }),
        state.activeLoans.end()
    );

    if (auto* br = dynamic_cast<const mvir::BranchTerm*>(block.terminator.get())) {
        if (auto* tTarget = findBlock(func, br->trueTarget)) checkDataflow(func, *tTarget, state, liveness, visited);
        if (auto* fTarget = findBlock(func, br->falseTarget)) checkDataflow(func, *fTarget, state, liveness, visited);
    } else if (auto* jmp = dynamic_cast<const mvir::JumpTerm*>(block.terminator.get())) {
        if (auto* tTarget = findBlock(func, jmp->target)) checkDataflow(func, *tTarget, state, liveness, visited);
    } else if (auto* sw = dynamic_cast<const mvir::SwitchTerm*>(block.terminator.get())) {
        if (auto* dTarget = findBlock(func, sw->defaultTarget)) checkDataflow(func, *dTarget, state, liveness, visited);
        for(const auto& c : sw->cases) {
            if (auto* cTarget = findBlock(func, c.second)) checkDataflow(func, *cTarget, state, liveness, visited);
        }
    }
}

Place BorrowChecker::resolvePlace(const mvir::Operand& op, const DataflowState& state) const {
    if (auto* locId = std::get_if<mvir::LocalId>(&op)) {
        auto it = state.placeMap.find(locId->name);
        if (it != state.placeMap.end()) {
            return it->second;
        }
    }
    return Place(op);
}

void BorrowChecker::checkInstruction(const mvir::Instruction& inst, DataflowState& state, const LivenessInfo& liveness) {
    SourceLocation fakeLoc{0, 0, 0}; 

    if (auto* idxInst = dynamic_cast<const mvir::IndexInst*>(&inst)) {
        Place basePlace = resolvePlace(idxInst->base, state);
        Place derefPlace = basePlace;
        derefPlace.projections.push_back(Projection{ProjectionKind::Deref, ""});
        state.placeMap[idxInst->dest.name] = derefPlace;
    }
    else if (auto* fieldInst = dynamic_cast<const mvir::FieldInst*>(&inst)) {
        Place basePlace = resolvePlace(fieldInst->base, state);
        Place fieldPlace = basePlace;
        fieldPlace.projections.push_back(Projection{ProjectionKind::Field, std::to_string(fieldInst->index)});
        state.placeMap[fieldInst->dest.name] = fieldPlace;
    }
    else if (auto* borrow = dynamic_cast<const mvir::BorrowInst*>(&inst)) {
        Place basePlace = resolvePlace(borrow->base, state);
        issueLoan(basePlace, borrow->isMutable, borrow->dest.name, fakeLoc, state);
        Place derefPlace = basePlace;
        derefPlace.projections.push_back(Projection{ProjectionKind::Deref, ""});
        state.placeMap[borrow->dest.name] = derefPlace;
    }
    else if (auto* load = dynamic_cast<const mvir::LoadInst*>(&inst)) {
        Place srcPlace = resolvePlace(load->ptr, state);
        checkAccess(srcPlace, false /* isMut */, fakeLoc, state);
    }
    else if (auto* store = dynamic_cast<const mvir::StoreInst*>(&inst)) {
        Place destPlace = resolvePlace(store->ptr, state);
        checkAccess(destPlace, true /* isMut */, fakeLoc, state);
    }
}

void BorrowChecker::issueLoan(const Place& place, bool isMut, const std::string& refId, SourceLocation loc, DataflowState& state) {
    for (const auto& loan : state.activeLoans) {
        if (loan.place.overlapsWith(place)) {
            if (loan.isMutable || isMut) {
                std::string msg = "Cannot borrow '" + place.toString() + "' as " + 
                                  (isMut ? "mutable" : "immutable") + 
                                  " because it is already borrowed as " + 
                                  (loan.isMutable ? "mutable" : "immutable");
                diag_.error(loc, msg);
                return;
            }
        }
    }
    state.activeLoans.push_back(Loan{nextLoanId_++, place, isMut, refId, loc});
}

void BorrowChecker::checkAccess(const Place& place, bool isMut, SourceLocation loc, const DataflowState& state) {
    for (const auto& loan : state.activeLoans) {
        if (loan.place.overlapsWith(place)) {
            if (isMut || loan.isMutable) {
                std::string msg = "Cannot " + std::string(isMut ? "mutate" : "access") + 
                                  " '" + place.toString() + "' because it is currently borrowed";
                diag_.error(loc, msg);
            }
        }
    }
}

} // namespace fl

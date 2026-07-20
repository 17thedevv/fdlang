#include "mellis/MiddleEnd/MVIROptimizer.h"
#include "mellis/MiddleEnd/LivenessAnalyzer.h"
#include <unordered_map>
#include <unordered_set>
#include <algorithm>

namespace fl {

// ConstantFoldingPass
bool ConstantFoldingPass::runOnFunction(mvir::Function& func) {
    bool changed = false;
    std::unordered_map<std::string, mvir::Operand> replacements;
    
    for (auto& block : func.blocks) {
        for (auto it = block->instructions.begin(); it != block->instructions.end(); ) {
            if (auto* alu = dynamic_cast<mvir::AluInst*>(it->get())) {
                if (auto* l = std::get_if<mvir::Number>(&alu->left)) {
                    if (auto* r = std::get_if<mvir::Number>(&alu->right)) {
                        int lv = std::stoi(l->value);
                        int rv = std::stoi(r->value);
                        int res = 0;
                        switch (alu->op) {
                            case mvir::AluOp::Add: res = lv + rv; break;
                            case mvir::AluOp::Sub: res = lv - rv; break;
                            case mvir::AluOp::Mul: res = lv * rv; break;
                            case mvir::AluOp::Div: if (rv != 0) res = lv / rv; break;
                            default: break; 
                        }
                        // We do not fold everything for MVP, but if we do:
                        replacements[alu->dest.name] = mvir::Number{std::to_string(res)};
                        it = block->instructions.erase(it);
                        changed = true;
                        continue;
                    }
                }
            }
            ++it;
        }
    }
    
    // Replace operands in remaining instructions
    auto replaceOperand = [&](mvir::Operand& op, const std::unordered_map<std::string, mvir::Operand>& reps) {
        if (auto* loc = std::get_if<mvir::LocalId>(&op)) {
            auto rit = reps.find(loc->name);
            if (rit != reps.end()) {
                op = rit->second;
            }
        }
    };
    
    for (auto& block : func.blocks) {
        for (auto& inst : block->instructions) {
            if (auto* store = dynamic_cast<mvir::StoreInst*>(inst.get())) {
                replaceOperand(store->value, replacements);
                replaceOperand(store->ptr, replacements);
            } else if (auto* load = dynamic_cast<mvir::LoadInst*>(inst.get())) {
                replaceOperand(load->ptr, replacements);
            } else if (auto* alu = dynamic_cast<mvir::AluInst*>(inst.get())) {
                replaceOperand(alu->left, replacements);
                replaceOperand(alu->right, replacements);
            }
            // Add more replacements if needed for full MVP
        }
        
        if (auto* branch = dynamic_cast<mvir::BranchTerm*>(block->terminator.get())) {
            replaceOperand(branch->condition, replacements);
        } else if (auto* ret = dynamic_cast<mvir::RetTerm*>(block->terminator.get())) {
            if (ret->value) replaceOperand(*ret->value, replacements);
        } else if (auto* sw = dynamic_cast<mvir::SwitchTerm*>(block->terminator.get())) {
            replaceOperand(sw->condition, replacements);
        }
    }
    return changed;
}

// ControlFlowSimplificationPass
bool ControlFlowSimplificationPass::runOnFunction(mvir::Function& func) {
    bool changed = false;
    for (auto& block : func.blocks) {
        if (auto* branch = dynamic_cast<mvir::BranchTerm*>(block->terminator.get())) {
            if (auto* bVal = std::get_if<mvir::Boolean>(&branch->condition)) {
                if (bVal->value) {
                    block->terminator = std::make_unique<mvir::JumpTerm>(branch->trueTarget);
                } else {
                    block->terminator = std::make_unique<mvir::JumpTerm>(branch->falseTarget);
                }
                changed = true;
            }
        }
    }
    return changed;
}

// UnreachableBlockEliminationPass
bool UnreachableBlockEliminationPass::runOnFunction(mvir::Function& func) {
    if (func.blocks.empty()) return false;
    
    std::unordered_set<std::string> reachable;
    std::vector<std::string> worklist;
    
    worklist.push_back(func.blocks[0]->label.name);
    reachable.insert(func.blocks[0]->label.name);
    
    auto getBlock = [&](const std::string& name) -> mvir::BasicBlock* {
        for (auto& b : func.blocks) if (b->label.name == name) return b.get();
        return nullptr;
    };
    
    while (!worklist.empty()) {
        std::string current = worklist.back();
        worklist.pop_back();
        
        mvir::BasicBlock* bb = getBlock(current);
        if (!bb) continue;
        
        if (auto* jump = dynamic_cast<mvir::JumpTerm*>(bb->terminator.get())) {
            if (reachable.insert(jump->target.name).second) worklist.push_back(jump->target.name);
        } else if (auto* branch = dynamic_cast<mvir::BranchTerm*>(bb->terminator.get())) {
            if (reachable.insert(branch->trueTarget.name).second) worklist.push_back(branch->trueTarget.name);
            if (reachable.insert(branch->falseTarget.name).second) worklist.push_back(branch->falseTarget.name);
        } else if (auto* sw = dynamic_cast<mvir::SwitchTerm*>(bb->terminator.get())) {
            if (reachable.insert(sw->defaultTarget.name).second) worklist.push_back(sw->defaultTarget.name);
            for (auto& c : sw->cases) {
                if (reachable.insert(c.second.name).second) worklist.push_back(c.second.name);
            }
        }
    }
    
    size_t oldSize = func.blocks.size();
    func.blocks.erase(std::remove_if(func.blocks.begin(), func.blocks.end(), [&](const std::unique_ptr<mvir::BasicBlock>& b) {
        return reachable.find(b->label.name) == reachable.end();
    }), func.blocks.end());
    
    return func.blocks.size() < oldSize;
}

// DeadCodeEliminationPass (Dataflow Liveness Based)
bool DeadCodeEliminationPass::runOnFunction(mvir::Function& func) {
    bool changed = false;
    
    // 1. Phn tch Liveness b?ng LivenessAnalyzer
    LivenessInfo liveness = LivenessAnalyzer::computeLiveness(func);
    
    // 2. Scan qua t?ng instruction \? tm Pure Instruction c?u ra (dest) khng cn s?ng \? di (LiveOut)
    for (auto& block : func.blocks) {
        for (auto it = block->instructions.begin(); it != block->instructions.end(); ) {
            bool isPure = false;
            std::string destName = "";
            
            if (auto* alu = dynamic_cast<mvir::AluInst*>(it->get())) { isPure = true; destName = alu->dest.name; }
            else if (auto* una = dynamic_cast<mvir::UnaryInst*>(it->get())) { isPure = true; destName = una->dest.name; }
            else if (auto* load = dynamic_cast<mvir::LoadInst*>(it->get())) { isPure = true; destName = load->dest.name; }
            else if (auto* idx = dynamic_cast<mvir::IndexInst*>(it->get())) { isPure = true; destName = idx->dest.name; }
            else if (auto* fld = dynamic_cast<mvir::FieldInst*>(it->get())) { isPure = true; destName = fld->dest.name; }
            else if (auto* bor = dynamic_cast<mvir::BorrowInst*>(it->get())) { isPure = true; destName = bor->dest.name; }
            else if (auto* cst = dynamic_cast<mvir::CastInst*>(it->get())) { isPure = true; destName = cst->dest.name; }
            else if (auto* loc = dynamic_cast<mvir::LocalInst*>(it->get())) { isPure = true; destName = loc->dest.name; }
            else if (auto* ex = dynamic_cast<mvir::ExtractInst*>(it->get())) { isPure = true; destName = ex->dest.name; }
            else if (auto* tag = dynamic_cast<mvir::TagInst*>(it->get())) { isPure = true; destName = tag->dest.name; }
            else if (auto* var = dynamic_cast<mvir::VariantInst*>(it->get())) { isPure = true; destName = var->dest.name; }
            else if (auto* sz = dynamic_cast<mvir::SizeofInst*>(it->get())) { isPure = true; destName = sz->dest.name; }
            else if (auto* al = dynamic_cast<mvir::AlignofInst*>(it->get())) { isPure = true; destName = al->dest.name; }

            // Ch : Tuy?t \?i KHNG match CallInst, VirtualCallInst v StoreInst vo \y (V\ chng Impure).
            
            bool canRemove = false;
            if (isPure && !destName.empty()) {
                // H?i b?n \? Liveness
                auto livIt = liveness.liveInstructions.find(destName);
                if (livIt != liveness.liveInstructions.end()) {
                    // N?u l?nh ny khng thu?c m?ng LiveOut c?a b?n thn n => Code dead!
                    if (livIt->second.find(it->get()) == livIt->second.end()) {
                        canRemove = true;
                    }
                } else {
                    // C?ng c th? n khng bao gi? \?c use nn khng vo Liveness!
                    canRemove = true; 
                }
            }
            
            if (canRemove) {
                it = block->instructions.erase(it);
                changed = true;
            } else {
                ++it;
            }
        }
    }
    
    return changed;
}

MVIROptimizer::MVIROptimizer(DiagnosticEngine& diag) : diag_(diag) {
    passes_.push_back(std::make_unique<ConstantFoldingPass>());
    passes_.push_back(std::make_unique<ControlFlowSimplificationPass>());
    passes_.push_back(std::make_unique<UnreachableBlockEliminationPass>());
    passes_.push_back(std::make_unique<DeadCodeEliminationPass>());
}

bool MVIROptimizer::optimize(mvir::Module& module) {
    bool anyChanged = false;
    for (auto& func : module.functions) {
        bool funcChanged = true;
        int iter = 0;
        while (funcChanged && iter < 10) { 
            funcChanged = false;
            for (auto& pass : passes_) {
                if (pass->runOnFunction(*func)) {
                    funcChanged = true;
                    anyChanged = true;
                }
            }
            iter++;
        }
    }
    return anyChanged;
}

} // namespace fl

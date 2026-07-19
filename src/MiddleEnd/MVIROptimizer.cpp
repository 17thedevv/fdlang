
#include "mellis/MiddleEnd/MVIROptimizer.h"
#include <iostream>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>

namespace fl {

// Helper to replace operands
static void replaceOperand(mvir::Operand& op, const std::unordered_map<std::string, mvir::Operand>& replacements) {
    if (auto* loc = std::get_if<mvir::LocalId>(&op)) {
        if (replacements.count(loc->name)) {
            op = replacements.at(loc->name);
        }
    }
}

// Helper to replace operands in an instruction
static void replaceOperandsInInst(mvir::Instruction& inst, const std::unordered_map<std::string, mvir::Operand>& replacements) {
    if (auto* load = dynamic_cast<mvir::LoadInst*>(&inst)) replaceOperand(load->ptr, replacements);
    else if (auto* store = dynamic_cast<mvir::StoreInst*>(&inst)) {
        replaceOperand(store->value, replacements);
        replaceOperand(store->ptr, replacements);
    }
    else if (auto* idx = dynamic_cast<mvir::IndexInst*>(&inst)) {
        replaceOperand(idx->base, replacements);
        replaceOperand(idx->index, replacements);
    }
    else if (auto* fld = dynamic_cast<mvir::FieldInst*>(&inst)) replaceOperand(fld->base, replacements);
    else if (auto* bor = dynamic_cast<mvir::BorrowInst*>(&inst)) replaceOperand(bor->base, replacements);
    else if (auto* cst = dynamic_cast<mvir::CastInst*>(&inst)) replaceOperand(cst->value, replacements);
    else if (auto* alu = dynamic_cast<mvir::AluInst*>(&inst)) {
        replaceOperand(alu->left, replacements);
        replaceOperand(alu->right, replacements);
    }
    else if (auto* una = dynamic_cast<mvir::UnaryInst*>(&inst)) replaceOperand(una->operand, replacements);
    else if (auto* ext = dynamic_cast<mvir::ExtractInst*>(&inst)) replaceOperand(ext->base, replacements);
    else if (auto* tag = dynamic_cast<mvir::TagInst*>(&inst)) replaceOperand(tag->base, replacements);
    else if (auto* var = dynamic_cast<mvir::VariantInst*>(&inst)) {
        for (auto& a : var->args) replaceOperand(a, replacements);
    }
    else if (auto* mto = dynamic_cast<mvir::MakeTraitObjectInst*>(&inst)) replaceOperand(mto->value, replacements);
    else if (auto* call = dynamic_cast<mvir::CallInst*>(&inst)) {
        replaceOperand(call->func, replacements);
        for (auto& a : call->args) replaceOperand(a, replacements);
    }
    else if (auto* vcall = dynamic_cast<mvir::VirtualCallInst*>(&inst)) {
        replaceOperand(vcall->receiver, replacements);
        for (auto& a : vcall->args) replaceOperand(a, replacements);
    }
    else if (auto* aw = dynamic_cast<mvir::AwaitInst*>(&inst)) replaceOperand(aw->futureVal, replacements);
}

// ConstantFoldingPass
bool ConstantFoldingPass::runOnFunction(mvir::Function& func) {
    bool changed = false;
    std::unordered_map<std::string, mvir::Operand> replacements;
    
    for (auto& block : func.blocks) {
        for (auto it = block->instructions.begin(); it != block->instructions.end(); ) {
            replaceOperandsInInst(**it, replacements);
            
            bool instRemoved = false;
            if (auto* alu = dynamic_cast<mvir::AluInst*>(it->get())) {
                auto* lNum = std::get_if<mvir::Number>(&alu->left);
                auto* rNum = std::get_if<mvir::Number>(&alu->right);
                if (lNum && rNum) {
                    try {
                        long long lVal = std::stoll(lNum->value);
                        long long rVal = std::stoll(rNum->value);
                        long long res = 0;
                        bool isBool = false;
                        switch (alu->op) {
                            case mvir::AluOp::Add: res = lVal + rVal; break;
                            case mvir::AluOp::Sub: res = lVal - rVal; break;
                            case mvir::AluOp::Mul: res = lVal * rVal; break;
                            case mvir::AluOp::Div: if (rVal != 0) res = lVal / rVal; break;
                            case mvir::AluOp::Eq: res = (lVal == rVal); isBool = true; break;
                            case mvir::AluOp::Ne: res = (lVal != rVal); isBool = true; break;
                            case mvir::AluOp::Lt: res = (lVal < rVal); isBool = true; break;
                            case mvir::AluOp::Le: res = (lVal <= rVal); isBool = true; break;
                            case mvir::AluOp::Gt: res = (lVal > rVal); isBool = true; break;
                            case mvir::AluOp::Ge: res = (lVal >= rVal); isBool = true; break;
                        }
                        if (isBool) {
                            replacements[alu->dest.name] = mvir::Boolean{res != 0};
                        } else {
                            replacements[alu->dest.name] = mvir::Number{std::to_string(res)};
                        }
                        it = block->instructions.erase(it);
                        instRemoved = true;
                        changed = true;
                    } catch (...) {
                        // ignore overflow
                    }
                }
            } else if (auto* una = dynamic_cast<mvir::UnaryInst*>(it->get())) {
                auto* valNum = std::get_if<mvir::Number>(&una->operand);
                auto* valBool = std::get_if<mvir::Boolean>(&una->operand);
                if (valNum && una->op == mvir::UnaryOp::Negate) {
                    try {
                        long long v = std::stoll(valNum->value);
                        replacements[una->dest.name] = mvir::Number{std::to_string(-v)};
                        it = block->instructions.erase(it);
                        instRemoved = true;
                        changed = true;
                    } catch (...) {}
                } else if (valBool && una->op == mvir::UnaryOp::Negate) { // assuming bool negation is negate instead of bitnot
                    replacements[una->dest.name] = mvir::Boolean{!valBool->value};
                    it = block->instructions.erase(it);
                    instRemoved = true;
                    changed = true;
                }
            }
            if (!instRemoved) ++it;
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

// DeadCodeEliminationPass
bool DeadCodeEliminationPass::runOnFunction(mvir::Function& func) {
    bool changed = false;
    std::unordered_map<std::string, int> useCounts;
    
    auto countUses = [&](const mvir::Operand& op) {
        if (auto* loc = std::get_if<mvir::LocalId>(&op)) useCounts[loc->name]++;
    };
    
    for (auto& block : func.blocks) {
        for (auto& inst : block->instructions) {
            if (auto* load = dynamic_cast<mvir::LoadInst*>(inst.get())) countUses(load->ptr);
            else if (auto* store = dynamic_cast<mvir::StoreInst*>(inst.get())) {
                countUses(store->value); countUses(store->ptr);
            }
            else if (auto* idx = dynamic_cast<mvir::IndexInst*>(inst.get())) {
                countUses(idx->base); countUses(idx->index);
            }
            else if (auto* fld = dynamic_cast<mvir::FieldInst*>(inst.get())) countUses(fld->base);
            else if (auto* bor = dynamic_cast<mvir::BorrowInst*>(inst.get())) countUses(bor->base);
            else if (auto* cst = dynamic_cast<mvir::CastInst*>(inst.get())) countUses(cst->value);
            else if (auto* alu = dynamic_cast<mvir::AluInst*>(inst.get())) {
                countUses(alu->left); countUses(alu->right);
            }
            else if (auto* una = dynamic_cast<mvir::UnaryInst*>(inst.get())) countUses(una->operand);
            else if (auto* ext = dynamic_cast<mvir::ExtractInst*>(inst.get())) countUses(ext->base);
            else if (auto* tag = dynamic_cast<mvir::TagInst*>(inst.get())) countUses(tag->base);
            else if (auto* var = dynamic_cast<mvir::VariantInst*>(inst.get())) {
                for (auto& a : var->args) countUses(a);
            }
            else if (auto* mto = dynamic_cast<mvir::MakeTraitObjectInst*>(inst.get())) countUses(mto->value);
            else if (auto* call = dynamic_cast<mvir::CallInst*>(inst.get())) {
                countUses(call->func);
                for (auto& a : call->args) countUses(a);
            }
            else if (auto* vcall = dynamic_cast<mvir::VirtualCallInst*>(inst.get())) {
                countUses(vcall->receiver);
                for (auto& a : vcall->args) countUses(a);
            }
            else if (auto* aw = dynamic_cast<mvir::AwaitInst*>(inst.get())) countUses(aw->futureVal);
        }
        
        if (auto* branch = dynamic_cast<mvir::BranchTerm*>(block->terminator.get())) {
            countUses(branch->condition);
        } else if (auto* ret = dynamic_cast<mvir::RetTerm*>(block->terminator.get())) {
            if (ret->value) countUses(*ret->value);
        } else if (auto* sw = dynamic_cast<mvir::SwitchTerm*>(block->terminator.get())) {
            countUses(sw->condition);
        }
    }
    
    // Now remove unused instructions that have side-effect free destinations
    for (auto& block : func.blocks) {
        for (auto it = block->instructions.begin(); it != block->instructions.end(); ) {
            bool canRemove = false;
            std::string destName = "";
            
            if (auto* alu = dynamic_cast<mvir::AluInst*>(it->get())) destName = alu->dest.name;
            else if (auto* una = dynamic_cast<mvir::UnaryInst*>(it->get())) destName = una->dest.name;
            else if (auto* load = dynamic_cast<mvir::LoadInst*>(it->get())) destName = load->dest.name;
            else if (auto* idx = dynamic_cast<mvir::IndexInst*>(it->get())) destName = idx->dest.name;
            else if (auto* fld = dynamic_cast<mvir::FieldInst*>(it->get())) destName = fld->dest.name;
            else if (auto* bor = dynamic_cast<mvir::BorrowInst*>(it->get())) destName = bor->dest.name;
            else if (auto* cst = dynamic_cast<mvir::CastInst*>(it->get())) destName = cst->dest.name;
            
            if (!destName.empty() && useCounts[destName] == 0) {
                canRemove = true;
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
        while (funcChanged && iter < 10) { // Max iterations to prevent infinite loops
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

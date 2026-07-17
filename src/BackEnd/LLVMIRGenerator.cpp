#include "fdlang/BackEnd/LLVMIRGenerator.h"
#include <iostream>

namespace fl {

LLVMIRGenerator::LLVMIRGenerator(llvm::LLVMContext& context, llvm::Module& module)
    : context_(context), module_(module), builder_(context) {}

bool LLVMIRGenerator::generate(const flir::Module* flirModule) {
    values_.clear();
    blocks_.clear();

    // Two-pass approach:
    // Pass 1: Declare all functions and create basic blocks to allow forward-branching.
    for (const auto& func : flirModule->functions) {
        createFunctionStructure(func.get());
    }

    // Pass 2: Emit instructions into the created basic blocks.
    for (const auto& func : flirModule->functions) {
        emitFunctionBody(func.get());
    }

    // Verify module correctness
    bool broken = llvm::verifyModule(module_, &llvm::errs());
    return !broken;
}

// ── Translation Helpers ──────────────────────────────────────────────────────

llvm::Type* LLVMIRGenerator::mapType(FLType type) {
    switch (type) {
        case FLType::Int: return llvm::Type::getInt32Ty(context_);
        case FLType::Bool: return llvm::Type::getInt1Ty(context_);
        case FLType::Unknown: return llvm::Type::getVoidTy(context_);
    }
    return llvm::Type::getVoidTy(context_);
}

llvm::Value* LLVMIRGenerator::mapOperand(const flir::Operand& op) {
    if (std::holds_alternative<flir::LocalId>(op)) {
        const auto& local = std::get<flir::LocalId>(op);
        auto it = values_.find(local.name);
        assert(it != values_.end() && "LocalId not found in environment");
        return it->second;
    } else if (std::holds_alternative<flir::GlobalId>(op)) {
        const auto& global = std::get<flir::GlobalId>(op);
        // Stripping '@' is handled implicitly by LLVM module symbol lookup
        std::string name = global.name.substr(1); // remove '@'
        llvm::Function* f = module_.getFunction(name);
        assert(f && "Global function not found");
        return f;
    } else if (std::holds_alternative<flir::Number>(op)) {
        const auto& num = std::get<flir::Number>(op);
        return llvm::ConstantInt::get(llvm::Type::getInt32Ty(context_), std::stoull(num.value), 10);
    } else if (std::holds_alternative<flir::Boolean>(op)) {
        const auto& b = std::get<flir::Boolean>(op);
        return llvm::ConstantInt::get(llvm::Type::getInt1Ty(context_), b.value ? 1 : 0);
    }
    assert(false && "Unknown operand type");
    return nullptr;
}

llvm::FunctionCallee LLVMIRGenerator::getOrDeclarePrint() {
    llvm::Function* f = module_.getFunction("print");
    if (!f) {
        llvm::Type* args[] = { llvm::Type::getInt32Ty(context_) };
        llvm::FunctionType* fType = llvm::FunctionType::get(llvm::Type::getVoidTy(context_), args, false);
        return module_.getOrInsertFunction("print", fType);
    }
    return f;
}

// ── Pass 1: Structure ────────────────────────────────────────────────────────

void LLVMIRGenerator::createFunctionStructure(const flir::Function* func) {
    // 1. Declare function
    std::string funcName = func->name.name.substr(1); // Strip '@'
    
    std::vector<llvm::Type*> paramTypes;
    for (const auto& param : func->params) {
        paramTypes.push_back(mapType(param.type));
    }
    
    llvm::FunctionType* fType = llvm::FunctionType::get(mapType(func->returnType), paramTypes, false);
    llvm::Function* llvmFunc = llvm::Function::Create(fType, llvm::Function::ExternalLinkage, funcName, module_);
    
    // Map parameters to local values
    size_t idx = 0;
    for (auto& arg : llvmFunc->args()) {
        values_[func->params[idx].id.name] = &arg;
        arg.setName(func->params[idx].id.name.substr(1)); // strip '%'
        idx++;
    }

    // 2. Create BasicBlocks
    for (const auto& block : func->blocks) {
        llvm::BasicBlock* bb = llvm::BasicBlock::Create(context_, block->label.name, llvmFunc);
        blocks_[block->label.name] = bb;
    }
}

// ── Pass 2: Instructions ─────────────────────────────────────────────────────

void LLVMIRGenerator::emitFunctionBody(const flir::Function* func) {
    for (const auto& block : func->blocks) {
        llvm::BasicBlock* bb = blocks_[block->label.name];
        builder_.SetInsertPoint(bb);
        
        for (const auto& inst : block->instructions) {
            emitInstruction(inst.get());
        }
        
        if (block->terminator) {
            emitTerminator(block->terminator.get());
        }
    }
}

void LLVMIRGenerator::emitInstruction(const flir::Instruction* inst) {
    if (auto* alloc = dynamic_cast<const flir::AllocaInst*>(inst)) {
        llvm::Type* ty = mapType(alloc->type);
        llvm::Value* val = builder_.CreateAlloca(ty, nullptr, alloc->dest.name.substr(1));
        values_[alloc->dest.name] = val;
        pointerTypes_[alloc->dest.name] = ty;
    }
    else if (auto* load = dynamic_cast<const flir::LoadInst*>(inst)) {
        llvm::Value* ptr = mapOperand(load->ptr);
        
        // Lookup allocated type
        std::string ptrName = std::get<flir::LocalId>(load->ptr).name;
        assert(pointerTypes_.count(ptrName) && "Pointer type not tracked");
        llvm::Type* pointeeTy = pointerTypes_[ptrName];
        
        llvm::Value* val = builder_.CreateLoad(pointeeTy, ptr, load->dest.name.substr(1));
        values_[load->dest.name] = val;
    }
    else if (auto* store = dynamic_cast<const flir::StoreInst*>(inst)) {
        llvm::Value* val = mapOperand(store->value);
        llvm::Value* ptr = mapOperand(store->ptr);
        builder_.CreateStore(val, ptr);
    }
    else if (auto* alu = dynamic_cast<const flir::AluInst*>(inst)) {
        llvm::Value* left = mapOperand(alu->left);
        llvm::Value* right = mapOperand(alu->right);
        llvm::Value* res = nullptr;
        
        switch (alu->op) {
            case flir::AluOp::Add: res = builder_.CreateAdd(left, right); break;
            case flir::AluOp::Sub: res = builder_.CreateSub(left, right); break;
            case flir::AluOp::Mul: res = builder_.CreateMul(left, right); break;
            case flir::AluOp::Div: res = builder_.CreateSDiv(left, right); break; // Signed division MVP
            case flir::AluOp::Eq:  res = builder_.CreateICmpEQ(left, right); break;
            case flir::AluOp::Ne:  res = builder_.CreateICmpNE(left, right); break;
            case flir::AluOp::Lt:  res = builder_.CreateICmpSLT(left, right); break;
            case flir::AluOp::Le:  res = builder_.CreateICmpSLE(left, right); break;
            case flir::AluOp::Gt:  res = builder_.CreateICmpSGT(left, right); break;
            case flir::AluOp::Ge:  res = builder_.CreateICmpSGE(left, right); break;
        }
        res->setName(alu->dest.name.substr(1));
        values_[alu->dest.name] = res;
    }
    else if (auto* call = dynamic_cast<const flir::CallInst*>(inst)) {
        llvm::FunctionCallee fcallee;
        if (call->func.name == "@print") {
            fcallee = getOrDeclarePrint();
        } else {
            llvm::Function* f = module_.getFunction(call->func.name.substr(1));
            assert(f && "Function not found");
            fcallee = f;
        }
        
        std::vector<llvm::Value*> args;
        for (const auto& arg : call->args) {
            args.push_back(mapOperand(arg));
        }
        
        llvm::Value* res = builder_.CreateCall(fcallee, args);
        if (call->dest) {
            res->setName(call->dest->name.substr(1));
            values_[call->dest->name] = res;
        }
    }
}

void LLVMIRGenerator::emitTerminator(const flir::Terminator* term) {
    if (auto* jump = dynamic_cast<const flir::JumpTerm*>(term)) {
        llvm::BasicBlock* target = blocks_[jump->target.name];
        builder_.CreateBr(target);
    }
    else if (auto* branch = dynamic_cast<const flir::BranchTerm*>(term)) {
        llvm::Value* cond = mapOperand(branch->condition);
        llvm::BasicBlock* trueBB = blocks_[branch->trueTarget.name];
        llvm::BasicBlock* falseBB = blocks_[branch->falseTarget.name];
        builder_.CreateCondBr(cond, trueBB, falseBB);
    }
    else if (auto* ret = dynamic_cast<const flir::RetTerm*>(term)) {
        if (ret->value) {
            llvm::Value* val = mapOperand(*(ret->value));
            builder_.CreateRet(val);
        } else {
            builder_.CreateRetVoid();
        }
    }
}

}

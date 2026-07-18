#include "mellis/BackEnd/LLVMIRGenerator.h"
#include <llvm/IR/Verifier.h>
#include <llvm/Support/raw_ostream.h>
#include <iostream>
#include <cassert>

namespace fl {

LLVMIRGenerator::LLVMIRGenerator(llvm::LLVMContext& context, llvm::Module& module, const SymbolTable& symTable)
    : context_(context), module_(module), builder_(context), symTable_(symTable) {}

bool LLVMIRGenerator::generate(const flir::Module* flirModule) {
    globalValues_.clear(); localValues_.clear(); pointerTypes_.clear();
    structTypes_.clear(); blocks_.clear();

    // 1. Declare Struct types (opaque first)
    for (const auto& tDecl : flirModule->typeDecls) {
        std::string structName = tDecl.name.substr(1);
        llvm::StructType* st = llvm::StructType::create(context_, structName);
        structTypes_[structName] = st;
    }
    // Set bodies for Struct types
    for (const auto& tDecl : flirModule->typeDecls) {
        std::string structName = tDecl.name.substr(1);
        llvm::StructType* st = structTypes_[structName];
        std::vector<llvm::Type*> fields;
        for (const auto& fType : tDecl.fields) {
            fields.push_back(mapType(fType));
        }
        st->setBody(fields);
    }

    // Pass 1: Declare all functions
    for (const auto& eFunc : flirModule->externFunctions) {
        std::string funcName = eFunc.name.name.substr(1); // strip '@'
        std::vector<llvm::Type*> paramTypes;
        for (const auto& pType : eFunc.paramTypes) {
            paramTypes.push_back(mapType(pType));
        }
        llvm::FunctionType* fType = llvm::FunctionType::get(mapType(eFunc.returnType), paramTypes, eFunc.isVariadic);
        llvm::Function::Create(fType, llvm::Function::ExternalLinkage, funcName, module_);
    }

    for (const auto& func : flirModule->functions) {
        std::string funcName = func->name.name.substr(1);
        std::vector<llvm::Type*> paramTypes;
        for (const auto& param : func->params) {
            paramTypes.push_back(mapType(param.type));
        }
        llvm::FunctionType* fType = llvm::FunctionType::get(mapType(func->returnType), paramTypes, false);
        llvm::Function::Create(fType, llvm::Function::ExternalLinkage, funcName, module_);
    }

    // 1.5 Global string literals
    for (const auto& gDecl : flirModule->globalDecls) {
        std::string name = gDecl.name.name.substr(1);
        if (!gDecl.stringLiteral.empty()) {
            llvm::Constant* strConst = llvm::ConstantDataArray::getString(context_, gDecl.stringLiteral, true);
            llvm::GlobalVariable* gv = new llvm::GlobalVariable(
                module_, strConst->getType(), true,
                llvm::GlobalValue::PrivateLinkage, strConst, name
            );
            globalValues_[gDecl.name.name] = gv;
        }
    }

    // Pass 2: Translate instructions
    for (const auto& func : flirModule->functions) {
        createFunctionStructure(func.get());
        emitFunctionBody(func.get());
    }

    //module_.print(llvm::errs(), nullptr); 
    bool broken = llvm::verifyModule(module_, &llvm::errs());
    return !broken;
}

llvm::Type* LLVMIRGenerator::mapType(const Type* type) {
    if (!type) return llvm::Type::getVoidTy(context_);
    
    if (auto* prim = dynamic_cast<const PrimitiveType*>(type)) {
        switch (prim->builtinKind) {
            case BuiltinKind::I8:
            case BuiltinKind::U8:
                return llvm::Type::getInt8Ty(context_);
            case BuiltinKind::I16:
            case BuiltinKind::U16:
                return llvm::Type::getInt16Ty(context_);
            case BuiltinKind::I32:
            case BuiltinKind::U32:
            case BuiltinKind::Char:
                return llvm::Type::getInt32Ty(context_);
            case BuiltinKind::I64:
            case BuiltinKind::U64:
                return llvm::Type::getInt64Ty(context_);
            case BuiltinKind::F32: return llvm::Type::getFloatTy(context_);
            case BuiltinKind::F64: return llvm::Type::getDoubleTy(context_);
            case BuiltinKind::Bool: return llvm::Type::getInt1Ty(context_);
            case BuiltinKind::Str: return llvm::PointerType::getUnqual(context_);
            default: return llvm::Type::getVoidTy(context_);
        }
    }
    if (auto* ptr = dynamic_cast<const PointerType*>(type)) {
        return llvm::PointerType::getUnqual(context_);
    }
    if (auto* ref = dynamic_cast<const ReferenceType*>(type)) {
        return llvm::PointerType::getUnqual(context_);
    }
    if (auto* st = dynamic_cast<const StructType*>(type)) {
        std::string name = "Struct<" + std::to_string(st->structSymbolId) + ">";
        if (structTypes_.count(name)) return structTypes_[name];
        for (auto& pair : structTypes_) {
            return pair.second;
        }
        return llvm::StructType::get(context_);
    }
    return llvm::Type::getVoidTy(context_);
}

llvm::Value* LLVMIRGenerator::mapOperand(const flir::Operand& op) {
    if (std::holds_alternative<flir::LocalId>(op)) {
        const auto& local = std::get<flir::LocalId>(op);
        auto it = localValues_.find(local.name);
        if (it != localValues_.end()) return it->second;
        assert(false && "LocalId not found in environment");
        return nullptr;
    } else if (std::holds_alternative<flir::GlobalId>(op)) {
        const auto& global = std::get<flir::GlobalId>(op);
        auto it = globalValues_.find(global.name);
        if (it != globalValues_.end()) return it->second;
        
        std::string name = global.name.substr(1);
        llvm::Function* f = module_.getFunction(name);
        if (f) return f;
        
        assert(false && "Global function/value not found");
        return nullptr;
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

void LLVMIRGenerator::createFunctionStructure(const flir::Function* func) {
    localValues_.clear();
    blocks_.clear();
    pointerTypes_.clear();
    
    std::string funcName = func->name.name.substr(1);
    llvm::Function* llvmFunc = module_.getFunction(funcName);
    
    size_t idx = 0;
    for (auto& arg : llvmFunc->args()) {
        localValues_[func->params[idx].id.name] = &arg;
        arg.setName(func->params[idx].id.name.substr(1));
        idx++;
    }

    for (const auto& block : func->blocks) {
        llvm::BasicBlock* bb = llvm::BasicBlock::Create(context_, block->label.name, llvmFunc);
        blocks_[block->label.name] = bb;
    }
}

void LLVMIRGenerator::emitFunctionBody(const flir::Function* func) {
    for (const auto& block : func->blocks) {
        llvm::BasicBlock* bb = blocks_[block->label.name];
        builder_.SetInsertPoint(bb); llvm::errs() << "Emitting for block " << block->label.name << " in function " << func->name.name << "\n";
        
        for (const auto& inst : block->instructions) {
            emitInstruction(inst.get());
        }
        
        if (block->terminator) {
            emitTerminator(block->terminator.get());
        } else {
            builder_.CreateRetVoid();
        }
    }
}

void LLVMIRGenerator::emitInstruction(const flir::Instruction* inst) {
    if (auto* alloc = dynamic_cast<const flir::AllocaInst*>(inst)) {
        llvm::Type* ty = mapType(alloc->type);
        llvm::Value* val = builder_.CreateAlloca(ty, nullptr, alloc->dest.name.substr(1));
        localValues_[alloc->dest.name] = val;
        pointerTypes_[alloc->dest.name] = ty;
    }
    else if (auto* load = dynamic_cast<const flir::LoadInst*>(inst)) {
        llvm::Value* ptr = mapOperand(load->ptr);
        std::string ptrName = std::get<flir::LocalId>(load->ptr).name;
        llvm::Type* pointeeTy = nullptr;
        if (pointerTypes_.count(ptrName)) {
            pointeeTy = pointerTypes_[ptrName];
        } else {
            pointeeTy = llvm::Type::getInt32Ty(context_);
        }
        
        llvm::Value* val = builder_.CreateLoad(pointeeTy, ptr, load->dest.name.substr(1));
        localValues_[load->dest.name] = val;
    }
    else if (auto* store = dynamic_cast<const flir::StoreInst*>(inst)) {
        llvm::Value* val = mapOperand(store->value);
        llvm::Value* ptr = mapOperand(store->ptr);
        builder_.CreateStore(val, ptr);
    }
    else if (auto* getptr = dynamic_cast<const flir::GetPtrInst*>(inst)) {
        llvm::Value* ptr = mapOperand(getptr->base);
        
        std::string ptrName = std::get<flir::LocalId>(getptr->base).name;
        llvm::Type* pointeeTy = nullptr;
        if (pointerTypes_.count(ptrName)) {
            pointeeTy = pointerTypes_[ptrName];
        } else {
            pointeeTy = llvm::Type::getInt32Ty(context_);
        }
        
        std::vector<llvm::Value*> indices;
        for (const auto& off : getptr->offsets) {
            indices.push_back(mapOperand(off));
        }
        
        llvm::Value* res = builder_.CreateGEP(pointeeTy, ptr, indices, getptr->dest.name.substr(1));
        localValues_[getptr->dest.name] = res;
        
        if (auto* st = llvm::dyn_cast<llvm::StructType>(pointeeTy)) {
            if (indices.size() == 2 && llvm::isa<llvm::ConstantInt>(indices[1])) {
                auto* ci = llvm::cast<llvm::ConstantInt>(indices[1]);
                uint64_t idx = ci->getZExtValue();
                if (idx < st->getNumElements()) {
                    pointerTypes_[getptr->dest.name] = st->getElementType(idx);
                }
            }
        }
    }
    else if (auto* castinst = dynamic_cast<const flir::CastInst*>(inst)) {
        llvm::Value* val = mapOperand(castinst->value);
        llvm::Value* res = builder_.CreateBitCast(val, mapType(castinst->targetType), castinst->dest.name.substr(1));
        localValues_[castinst->dest.name] = res;
    }
    else if (auto* alu = dynamic_cast<const flir::AluInst*>(inst)) {
        llvm::Value* left = mapOperand(alu->left);
        llvm::Value* right = mapOperand(alu->right);
        llvm::Value* res = nullptr;
        
        switch (alu->op) {
            case flir::AluOp::Add: res = builder_.CreateAdd(left, right); break;
            case flir::AluOp::Sub: res = builder_.CreateSub(left, right); break;
            case flir::AluOp::Mul: res = builder_.CreateMul(left, right); break;
            case flir::AluOp::Div: res = builder_.CreateSDiv(left, right); break;
            case flir::AluOp::Eq:  res = builder_.CreateICmpEQ(left, right); break;
            case flir::AluOp::Ne:  res = builder_.CreateICmpNE(left, right); break;
            case flir::AluOp::Lt:  res = builder_.CreateICmpSLT(left, right); break;
            case flir::AluOp::Le:  res = builder_.CreateICmpSLE(left, right); break;
            case flir::AluOp::Gt:  res = builder_.CreateICmpSGT(left, right); break;
            case flir::AluOp::Ge:  res = builder_.CreateICmpSGE(left, right); break;
        }
        res->setName(alu->dest.name.substr(1));
        localValues_[alu->dest.name] = res;
    }
    else if (auto* call = dynamic_cast<const flir::CallInst*>(inst)) {
        llvm::FunctionCallee fcallee;
        if (std::holds_alternative<flir::GlobalId>(call->func) && std::get<flir::GlobalId>(call->func).name == "@print") {
            fcallee = getOrDeclarePrint();
        } else {
            fcallee = module_.getFunction(std::get<flir::GlobalId>(call->func).name.substr(1));
            assert(fcallee && "Function not found");
        }
        
        std::vector<llvm::Value*> args;
        for (const auto& arg : call->args) {
            args.push_back(mapOperand(arg));
        }
        
        llvm::Value* res = builder_.CreateCall(fcallee, args);
        if (call->dest && !res->getType()->isVoidTy()) {
            res->setName(call->dest->name.substr(1));
            localValues_[call->dest->name] = res;
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


// =============================================================================
// mellis/BackEnd/LLVMIRGenerator.h
//
// LLVMIRGenerator — Lowering pass from FLIR to LLVM IR.
//
// Responsibilities:
//   - Consume a flir::Module and produce a well-formed llvm::Module.
//   - Pure translation: maps types, operands, and instructions 1:1.
//
// Constraints:
//   - No optimizations, no dead code elimination.
//   - Semantic analysis is forbidden (no access to AST, Resolver, TypeChecker).
//   - Output must pass llvm::verifyModule.
// =============================================================================

#pragma once

#include "mellis/MiddleEnd/FLIR.h"
#include "mellis/MiddleEnd/SymbolTable.h"
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Verifier.h>
#include <string>
#include <unordered_map>
#include <memory>

namespace fl {

class LLVMIRGenerator {
public:
    explicit LLVMIRGenerator(llvm::LLVMContext& context, llvm::Module& module, const SymbolTable& symTable);

    /// Generate LLVM IR from the given FLIR module.
    ///
    /// @return true if generation succeeded and passed verification.
    bool generate(const flir::Module* flirModule);

private:
    llvm::LLVMContext& context_;
    llvm::Module& module_;
    llvm::IRBuilder<> builder_;
    const SymbolTable& symTable_;

    // ── Environments ─────────────────────────────────────────────────────────
    
    // Maps FLIR LocalId ("%0") to the allocated LLVM Value.
    std::unordered_map<std::string, llvm::Value*> globalValues_;
    std::unordered_map<std::string, llvm::Value*> localValues_;
    
    // Maps FLIR LocalId to its allocated LLVM Type (needed for LLVM 15+ opaque pointers)
    std::unordered_map<std::string, llvm::Type*> pointerTypes_;
    
    // Maps Struct name ("Foo") to the LLVM StructType.
    std::unordered_map<std::string, llvm::StructType*> structTypes_;
    
    // Maps FLIR LabelId ("bb0") to the LLVM BasicBlock.
    std::unordered_map<std::string, llvm::BasicBlock*> blocks_;

    // ── Translation Helpers ──────────────────────────────────────────────────

    llvm::Type* mapType(const Type* type);
    llvm::Value* mapOperand(const flir::Operand& op);
    
    // Pass 1: Declare functions and blocks
    void createFunctionStructure(const flir::Function* func);
    
    // Pass 2: Emit instructions
    void emitFunctionBody(const flir::Function* func);
    void emitInstruction(const flir::Instruction* inst);
    void emitTerminator(const flir::Terminator* term);
    
    // External declarations
    llvm::FunctionCallee getOrDeclarePrint();
};

} // namespace fl

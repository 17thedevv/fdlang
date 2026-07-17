#include "fdlang/MiddleEnd/FLIR.h"
#include <sstream>

namespace fl {
namespace flir {

// =============================================================================
// 1. Types
// =============================================================================

std::string formatType(FLType type) {
    switch (type) {
        case FLType::Int:  return "i32";
        case FLType::Bool: return "bool";
        case FLType::Unknown: return "void"; // Fallback for void functions
    }
    return "void";
}

// =============================================================================
// 2. Operands
// =============================================================================

std::string toString(const Operand& op) {
    return std::visit([](auto&& arg) -> std::string {
        return arg.toString();
    }, op);
}

// =============================================================================
// 3. Instructions
// =============================================================================

std::string AllocaInst::toString() const {
    return dest.toString() + " = alloca " + formatType(type);
}

std::string LoadInst::toString() const {
    return dest.toString() + " = load " + flir::toString(ptr);
}

std::string StoreInst::toString() const {
    return "store " + flir::toString(value) + ", " + flir::toString(ptr);
}

std::string formatAluOp(AluOp op) {
    switch (op) {
        case AluOp::Add: return "add";
        case AluOp::Sub: return "sub";
        case AluOp::Mul: return "mul";
        case AluOp::Div: return "div";
        case AluOp::Eq:  return "eq";
        case AluOp::Ne:  return "ne";
        case AluOp::Lt:  return "lt";
        case AluOp::Le:  return "le";
        case AluOp::Gt:  return "gt";
        case AluOp::Ge:  return "ge";
    }
    return "unknown_alu_op";
}

std::string AluInst::toString() const {
    return dest.toString() + " = " + formatAluOp(op) + " " +
           flir::toString(left) + ", " + flir::toString(right);
}

std::string CallInst::toString() const {
    std::string res;
    if (dest) {
        res += dest->toString() + " = ";
    }
    res += "call " + func.toString() + "(";
    for (size_t i = 0; i < args.size(); ++i) {
        res += flir::toString(args[i]);
        if (i < args.size() - 1) {
            res += ", ";
        }
    }
    res += ")";
    return res;
}

// =============================================================================
// 4. Terminators
// =============================================================================

std::string JumpTerm::toString() const {
    return "jump " + target.toString();
}

std::string BranchTerm::toString() const {
    return "branch " + flir::toString(condition) + ", " +
           trueTarget.toString() + ", " + falseTarget.toString();
}

std::string RetTerm::toString() const {
    if (value) {
        return "ret " + flir::toString(*value);
    }
    return "ret";
}

// =============================================================================
// 5. Structure
// =============================================================================

std::string BasicBlock::toString() const {
    std::string res = label.toString() + ":\n";
    for (const auto& inst : instructions) {
        res += "  " + inst->toString() + "\n";
    }
    if (terminator) {
        res += "  " + terminator->toString() + "\n";
    }
    return res;
}

std::string Param::toString() const {
    return formatType(type) + " " + id.toString();
}

std::string Function::toString() const {
    std::string res = "func " + name.toString() + "(";
    for (size_t i = 0; i < params.size(); ++i) {
        res += params[i].toString();
        if (i < params.size() - 1) {
            res += ", ";
        }
    }
    res += ") -> " + formatType(returnType) + " {\n";
    for (const auto& bb : blocks) {
        res += bb->toString();
    }
    res += "}\n";
    return res;
}

std::string Module::toString() const {
    std::string res;
    for (const auto& func : functions) {
        res += func->toString() + "\n";
    }
    return res;
}

} // namespace flir
} // namespace fl

// =============================================================================
// fdlang/MiddleEnd/FLIR.h
//
// FreedomLanguage Intermediate Representation (FLIR).
//
// FLIR is a high-level representation, directly mirroring the specification
// in `docs/flir.md`. It strictly adheres to:
//   - Non-SSA for variables (using explicit alloca/load/store)
//   - Basic Blocks ending in a single terminator
//   - Strict type annotations
//
// This layer is pure data. The AST-to-FLIR translation logic belongs
// to FLIRGenerator.
// =============================================================================

#pragma once

#include "fdlang/Core/FLType.h"
#include <string>
#include <vector>
#include <memory>
#include <variant>
#include <optional>

namespace fl {
namespace flir {

// =============================================================================
// 1. Types
// =============================================================================

/// Convert FLType (from AST/TypeChecker) to a string representation matching
/// `docs/flir.md` (e.g., "i32", "bool", "void").
std::string formatType(FLType type);

// =============================================================================
// 2. Operands
// =============================================================================

struct LocalId {
    std::string name; // e.g., "%0", "%x"
    std::string toString() const { return name; }
};

struct GlobalId {
    std::string name; // e.g., "@main"
    std::string toString() const { return name; }
};

struct LabelId {
    std::string name; // e.g., "bb0"
    std::string toString() const { return name; }
};

struct Number {
    std::string value; // "42"
    std::string toString() const { return value; }
};

struct Boolean {
    bool value;
    std::string toString() const { return value ? "true" : "false"; }
};

using Operand = std::variant<LocalId, GlobalId, Number, Boolean>;

std::string toString(const Operand& op);

// =============================================================================
// 3. Instructions
// =============================================================================

struct Instruction {
    virtual ~Instruction() = default;
    virtual std::string toString() const = 0;
};

// ── Memory Instructions ──────────────────────────────────────────────────────

struct AllocaInst : public Instruction {
    LocalId dest;
    FLType type;

    AllocaInst(LocalId d, FLType t) : dest(std::move(d)), type(t) {}
    std::string toString() const override;
};

struct LoadInst : public Instruction {
    LocalId dest;
    Operand ptr;

    LoadInst(LocalId d, Operand p) : dest(std::move(d)), ptr(std::move(p)) {}
    std::string toString() const override;
};

struct StoreInst : public Instruction {
    Operand value;
    Operand ptr;

    StoreInst(Operand v, Operand p) : value(std::move(v)), ptr(std::move(p)) {}
    std::string toString() const override;
};

// ── ALU Instructions ─────────────────────────────────────────────────────────

enum class AluOp {
    Add, Sub, Mul, Div,
    Eq, Ne, Lt, Le, Gt, Ge
};

std::string formatAluOp(AluOp op);

struct AluInst : public Instruction {
    LocalId dest;
    AluOp op;
    Operand left;
    Operand right;

    AluInst(LocalId d, AluOp o, Operand l, Operand r)
        : dest(std::move(d)), op(o), left(std::move(l)), right(std::move(r)) {}
    std::string toString() const override;
};

// ── Call Instruction ─────────────────────────────────────────────────────────

struct CallInst : public Instruction {
    std::optional<LocalId> dest;
    GlobalId func;
    std::vector<Operand> args;

    CallInst(std::optional<LocalId> d, GlobalId f, std::vector<Operand> a)
        : dest(std::move(d)), func(std::move(f)), args(std::move(a)) {}
    std::string toString() const override;
};

// =============================================================================
// 4. Terminators
// =============================================================================

struct Terminator {
    virtual ~Terminator() = default;
    virtual std::string toString() const = 0;
};

struct JumpTerm : public Terminator {
    LabelId target;

    explicit JumpTerm(LabelId t) : target(std::move(t)) {}
    std::string toString() const override;
};

struct BranchTerm : public Terminator {
    Operand condition;
    LabelId trueTarget;
    LabelId falseTarget;

    BranchTerm(Operand cond, LabelId t, LabelId f)
        : condition(std::move(cond)), trueTarget(std::move(t)), falseTarget(std::move(f)) {}
    std::string toString() const override;
};

struct RetTerm : public Terminator {
    std::optional<Operand> value;

    explicit RetTerm(std::optional<Operand> v = std::nullopt) : value(std::move(v)) {}
    std::string toString() const override;
};

// =============================================================================
// 5. Structure (BasicBlocks, Functions, Module)
// =============================================================================

struct BasicBlock {
    LabelId label;
    std::vector<std::unique_ptr<Instruction>> instructions;
    std::unique_ptr<Terminator> terminator;

    explicit BasicBlock(LabelId l) : label(std::move(l)) {}
    std::string toString() const;
};

struct Param {
    FLType type;
    LocalId id;
    
    std::string toString() const;
};

struct Function {
    GlobalId name;
    std::vector<Param> params;
    FLType returnType;
    std::vector<std::unique_ptr<BasicBlock>> blocks;

    Function(GlobalId n, FLType ret) : name(std::move(n)), returnType(ret) {}
    std::string toString() const;
};

struct Module {
    std::vector<std::unique_ptr<Function>> functions;

    std::string toString() const;
};

} // namespace flir
} // namespace fl

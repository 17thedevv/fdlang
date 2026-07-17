#include "fdlang/MiddleEnd/FLIRGenerator.h"
#include "fdlang/AST/ASTNode.h"
#include "fdlang/AST/ExprNode.h"
#include "fdlang/AST/StmtNode.h"
#include <cassert>

namespace fl {

FLIRGenerator::FLIRGenerator(SymbolTable& symTable, TypeChecker& typeChecker)
    : table_(symTable), typeChecker_(typeChecker) {}

std::unique_ptr<flir::Module> FLIRGenerator::generate(ProgramNode* program) {
    module_ = std::make_unique<flir::Module>();
    nextLocalId_ = 0;
    nextLabelId_ = 0;
    varAllocas_.clear();

    program->accept(this);

    return std::move(module_);
}

// ── Helpers ──────────────────────────────────────────────────────────────────

flir::LocalId FLIRGenerator::nextLocal() {
    return flir::LocalId{"%" + std::to_string(nextLocalId_++)};
}

flir::LabelId FLIRGenerator::nextLabel(const std::string& prefix) {
    return flir::LabelId{prefix + std::to_string(nextLabelId_++)};
}

void FLIRGenerator::terminateBlock(std::unique_ptr<flir::Terminator> term) {
    if (currentBlock_ && !currentBlock_->terminator) {
        currentBlock_->terminator = std::move(term);
    }
}

void FLIRGenerator::startBlock(flir::LabelId label) {
    auto bb = std::make_unique<flir::BasicBlock>(std::move(label));
    currentBlock_ = bb.get();
    currentFunction_->blocks.push_back(std::move(bb));
}

flir::Operand FLIRGenerator::evaluate(ExprNode* expr) {
    expr->accept(this);
    return lastEvaluatedOperand_;
}

// ── ASTVisitor: Statements ───────────────────────────────────────────────────

void FLIRGenerator::visit(ProgramNode* node) {
    // MVP: wrap everything in @main() -> void
    auto mainFunc = std::make_unique<flir::Function>(flir::GlobalId{"@main"}, FLType::Unknown);
    currentFunction_ = mainFunc.get();
    module_->functions.push_back(std::move(mainFunc));

    // Start entry block
    startBlock(nextLabel());

    for (auto& stmt : node->statements) {
        stmt->accept(this);
    }

    // Terminate the last block of main if not already terminated
    terminateBlock(std::make_unique<flir::RetTerm>());
    
    currentFunction_ = nullptr;
    currentBlock_ = nullptr;
}

void FLIRGenerator::visit(VarDeclStmt* node) {
    // Variable Declaration:
    // 1. Allocate space on stack: %id = alloca type
    // 2. If initialized, evaluate RHS and store: store val, %id

    FLType varType = typeChecker_.typeOf(node->symbolId);
    
    flir::LocalId ptr = nextLocal();
    currentBlock_->instructions.push_back(
        std::make_unique<flir::AllocaInst>(ptr, varType)
    );
    
    // Save pointer location to mapping
    varAllocas_[node->symbolId] = ptr;

    if (node->initializer) {
        flir::Operand initVal = evaluate(node->initializer.get());
        currentBlock_->instructions.push_back(
            std::make_unique<flir::StoreInst>(initVal, ptr)
        );
    }
}

void FLIRGenerator::visit(AssignStmtNode* node) {
    // Assignment:
    // 1. Evaluate RHS
    // 2. Load pointer from symbol mapping
    // 3. store val, %id

    flir::Operand val = evaluate(node->value.get());
    
    assert(varAllocas_.count(node->symbolId) && "AssignStmtNode symbolId not allocated");
    flir::LocalId ptr = varAllocas_[node->symbolId];

    currentBlock_->instructions.push_back(
        std::make_unique<flir::StoreInst>(val, ptr)
    );
}

void FLIRGenerator::visit(BlockStmtNode* node) {
    for (auto& stmt : node->statements) {
        stmt->accept(this);
    }
}

void FLIRGenerator::visit(IfStmtNode* node) {
    flir::Operand cond = evaluate(node->condition.get());

    flir::LabelId thenLbl = nextLabel("then");
    flir::LabelId elseLbl = node->elseBranch ? nextLabel("else") : flir::LabelId{""};
    flir::LabelId mergeLbl = nextLabel("merge");

    if (!node->elseBranch) {
        elseLbl = mergeLbl;
    }

    terminateBlock(std::make_unique<flir::BranchTerm>(cond, thenLbl, elseLbl));

    // Then block
    startBlock(thenLbl);
    node->thenBranch->accept(this);
    terminateBlock(std::make_unique<flir::JumpTerm>(mergeLbl));

    // Else block
    if (node->elseBranch) {
        startBlock(elseLbl);
        node->elseBranch->accept(this);
        terminateBlock(std::make_unique<flir::JumpTerm>(mergeLbl));
    }

    // Merge block
    startBlock(mergeLbl);
}

void FLIRGenerator::visit(WhileStmtNode* node) {
    flir::LabelId condLbl = nextLabel("while_cond");
    flir::LabelId bodyLbl = nextLabel("while_body");
    flir::LabelId endLbl = nextLabel("while_end");

    terminateBlock(std::make_unique<flir::JumpTerm>(condLbl));

    // Condition block
    startBlock(condLbl);
    flir::Operand cond = evaluate(node->condition.get());
    terminateBlock(std::make_unique<flir::BranchTerm>(cond, bodyLbl, endLbl));

    // Body block
    startBlock(bodyLbl);
    node->body->accept(this);
    terminateBlock(std::make_unique<flir::JumpTerm>(condLbl));

    // End block
    startBlock(endLbl);
}

void FLIRGenerator::visit(PrintStmtNode* node) {
    flir::Operand val = evaluate(node->value.get());
    std::vector<flir::Operand> args = {val};
    
    currentBlock_->instructions.push_back(
        std::make_unique<flir::CallInst>(std::nullopt, flir::GlobalId{"@print"}, args)
    );
}

// ── ASTVisitor: Expressions ──────────────────────────────────────────────────

void FLIRGenerator::visit(NumberExpr* node) {
    lastEvaluatedOperand_ = flir::Number{std::string(node->value)};
}

void FLIRGenerator::visit(BooleanExpr* node) {
    lastEvaluatedOperand_ = flir::Boolean{node->value};
}

void FLIRGenerator::visit(IdentifierExpr* node) {
    // Identifier usage:
    // %val = load %ptr
    assert(varAllocas_.count(node->symbolId) && "IdentifierExpr symbolId not allocated");
    flir::LocalId ptr = varAllocas_[node->symbolId];
    flir::LocalId dest = nextLocal();

    currentBlock_->instructions.push_back(
        std::make_unique<flir::LoadInst>(dest, ptr)
    );

    lastEvaluatedOperand_ = dest;
}

void FLIRGenerator::visit(BinaryExpr* node) {
    flir::Operand left = evaluate(node->left.get());
    flir::Operand right = evaluate(node->right.get());

    flir::AluOp op;
    if (node->op == "+") op = flir::AluOp::Add;
    else if (node->op == "-") op = flir::AluOp::Sub;
    else if (node->op == "*") op = flir::AluOp::Mul;
    else if (node->op == "/") op = flir::AluOp::Div;
    else if (node->op == "==") op = flir::AluOp::Eq;
    else if (node->op == "!=") op = flir::AluOp::Ne;
    else if (node->op == "<") op = flir::AluOp::Lt;
    else if (node->op == "<=") op = flir::AluOp::Le;
    else if (node->op == ">") op = flir::AluOp::Gt;
    else if (node->op == ">=") op = flir::AluOp::Ge;
    else {
        assert(false && "Unknown binary operator");
        op = flir::AluOp::Add;
    }

    flir::LocalId dest = nextLocal();
    currentBlock_->instructions.push_back(
        std::make_unique<flir::AluInst>(dest, op, left, right)
    );

    lastEvaluatedOperand_ = dest;
}

} // namespace fl

#include "mellis/MiddleEnd/FLIRGenerator.h"
#include "mellis/AST/ASTNode.h"
#include "mellis/AST/ExprNode.h"
#include "mellis/AST/StmtNode.h"
#include "mellis/AST/DeclNode.h"
#include "mellis/AST/ProgramNode.h"
#include "mellis/AST/PatternNode.h"
#include <cassert>
#include <iostream>

namespace fl {

FLIRGenerator::FLIRGenerator(SymbolTable& symTable, TypeChecker& typeChecker)
    : table_(symTable), typeChecker_(typeChecker) {}

std::unique_ptr<flir::Module> FLIRGenerator::generate(ProgramNode& program) {
    module_ = std::make_unique<flir::Module>();
    nextLocalId_ = 0;
    nextLabelId_ = 0;
    varAllocas_.clear();
    evalMode_ = EvalMode::RValue;

    visit(program);

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

void FLIRGenerator::resetFunctionState() {
    nextLocalId_ = 0;
    nextLabelId_ = 0;
    varAllocas_.clear();
    currentBlock_ = nullptr;
}

flir::Operand FLIRGenerator::evaluateRValue(ExprNode& expr) {
    auto oldMode = evalMode_;
    evalMode_ = EvalMode::RValue;
    expr.accept(*this);
    evalMode_ = oldMode;
    return lastEvaluatedOperand_;
}

flir::Operand FLIRGenerator::evaluateLValue(ExprNode& expr) {
    auto oldMode = evalMode_;
    evalMode_ = EvalMode::LValue;
    expr.accept(*this);
    evalMode_ = oldMode;
    return lastEvaluatedOperand_;
}

// ── ASTVisitor: Statements ───────────────────────────────────────────────────

void FLIRGenerator::visit(ProgramNode& node) {
    for (auto& item : node.items) {
        item->accept(*this);
    }
}

void FLIRGenerator::visit(VarDeclNode& node) {
    // Variable Declaration:
    // 1. Allocate space on stack: %id = alloca type
    // 2. If initialized, evaluate RHS and store: store val, %id

    const Type* varType = typeChecker_.typeOf(node.symbolId);
    
    flir::LocalId ptr = nextLocal();
    currentBlock_->instructions.push_back(
        std::make_unique<flir::AllocaInst>(ptr, varType)
    );
    
    // Save pointer location to mapping
    varAllocas_[node.symbolId] = ptr;

    if (!scopeStack_.empty()) {
        scopeStack_.back().push_back(ptr);
    }

    if (node.initializer) {
        flir::Operand initVal = evaluateRValue(*node.initializer);
        currentBlock_->instructions.push_back(
            std::make_unique<flir::StoreInst>(initVal, ptr)
        );
    }
}

void FLIRGenerator::visit(AssignExpr& node) {
    // Assignment:
    // 1. Evaluate RHS
    // 2. Load pointer from symbol mapping
    // 3. store val, %id

    flir::Operand val = evaluateRValue(*node.value);
    flir::Operand ptr = evaluateLValue(*node.lvalue);

    currentBlock_->instructions.push_back(
        std::make_unique<flir::StoreInst>(val, ptr)
    );
    
    lastEvaluatedOperand_ = val;
}

void FLIRGenerator::visit(BlockStmtNode& node) {
    currentBlock_->instructions.push_back(std::make_unique<flir::BeginScopeInst>());
    for (auto& stmt : node.body) {
        stmt->accept(*this);
    }
    currentBlock_->instructions.push_back(std::make_unique<flir::EndScopeInst>());
}

void FLIRGenerator::visit(IfStmtNode& node) {
    flir::Operand cond = evaluateRValue(*node.condition);

    flir::LabelId thenLbl = nextLabel("then");
    flir::LabelId elseLbl = node.elseBranch ? nextLabel("else") : flir::LabelId{""};
    flir::LabelId mergeLbl = nextLabel("merge");

    if (!node.elseBranch) {
        elseLbl = mergeLbl;
    }

    terminateBlock(std::make_unique<flir::BranchTerm>(cond, thenLbl, elseLbl));

    // Then block
    startBlock(thenLbl);
    node.thenBranch->accept(*this);
    terminateBlock(std::make_unique<flir::JumpTerm>(mergeLbl));

    // Else block
    if (node.elseBranch) {
        startBlock(elseLbl);
        node.elseBranch->accept(*this);
        terminateBlock(std::make_unique<flir::JumpTerm>(mergeLbl));
    }

    // Merge block
    startBlock(mergeLbl);
}

void FLIRGenerator::visit(WhileStmtNode& node) {
    flir::LabelId condLbl = nextLabel("while_cond");
    flir::LabelId bodyLbl = nextLabel("while_body");
    flir::LabelId endLbl = nextLabel("while_end");

    terminateBlock(std::make_unique<flir::JumpTerm>(condLbl));

    // Condition block
    startBlock(condLbl);
    flir::Operand cond = evaluateRValue(*node.condition);
    terminateBlock(std::make_unique<flir::BranchTerm>(cond, bodyLbl, endLbl));

    // Body block
    startBlock(bodyLbl);
    node.body->accept(*this);
    terminateBlock(std::make_unique<flir::JumpTerm>(condLbl));

    // End block
    startBlock(endLbl);
}

// ── ASTVisitor: Expressions ──────────────────────────────────────────────────

void FLIRGenerator::visit(LiteralExpr& node) {
    if (node.kind == LiteralKind::Integer) {
        lastEvaluatedOperand_ = flir::Number{std::string(node.rawText)};
    } else if (node.kind == LiteralKind::Bool) {
        lastEvaluatedOperand_ = flir::Boolean{std::string(node.rawText) == "true"};
    } else if (node.kind == LiteralKind::Str) {
        static int stringCounter = 1;
        std::string globalIdStr = "@str_" + std::to_string(stringCounter++);
        flir::GlobalId gid{globalIdStr};

        flir::GlobalDecl globalDecl;
        globalDecl.name = gid;
        globalDecl.type = node.inferredType;
        
        std::string rawStr = std::string(node.rawText);
        if (rawStr.length() >= 2 && rawStr.front() == '"' && rawStr.back() == '"') {
            rawStr = rawStr.substr(1, rawStr.length() - 2);
        }
        // Handle basic escapes (like \n, \t)
        std::string unescaped;
        for (size_t i = 0; i < rawStr.length(); ++i) {
            if (rawStr[i] == '\\' && i + 1 < rawStr.length()) {
                switch(rawStr[i+1]) {
                    case 'n': unescaped += '\n'; break;
                    case 't': unescaped += '\t'; break;
                    case 'r': unescaped += '\r'; break;
                    case '0': unescaped += '\0'; break;
                    case '\\': unescaped += '\\'; break;
                    case '"': unescaped += '"'; break;
                    default: unescaped += rawStr[i+1]; break;
                }
                i++;
            } else {
                unescaped += rawStr[i];
            }
        }
        
        globalDecl.stringLiteral = unescaped + '\0'; // Add null terminator
        module_->globalDecls.push_back(globalDecl);
        
        lastEvaluatedOperand_ = gid;
    } else {
        lastEvaluatedOperand_ = flir::Number{"0"}; // Default stub for others
    }
}

void FLIRGenerator::visit(IdentifierExpr& node) {
    if (!varAllocas_.count(node.resolvedSymbol)) {
        std::cerr << "Missing symbolId in varAllocas_: " << node.segments.back() << " (ID: " << node.resolvedSymbol << ")\n";
        assert(false && "Identifier symbolId not allocated");
    }
    flir::LocalId ptr = varAllocas_[node.resolvedSymbol];

    if (evalMode_ == EvalMode::LValue) {
        // Return pointer directly
        lastEvaluatedOperand_ = ptr;
    } else {
        // Evaluate to value
        flir::LocalId dest = nextLocal();
        currentBlock_->instructions.push_back(
            std::make_unique<flir::LoadInst>(dest, ptr)
        );
        lastEvaluatedOperand_ = dest;
    }
}

void FLIRGenerator::visit(BinaryExpr& node) {
    flir::Operand left = evaluateRValue(*node.left);
    flir::Operand right = evaluateRValue(*node.right);

    flir::AluOp op;
    if (node.op == BinaryOp::Add) op = flir::AluOp::Add;
    else if (node.op == BinaryOp::Sub) op = flir::AluOp::Sub;
    else if (node.op == BinaryOp::Mul) op = flir::AluOp::Mul;
    else if (node.op == BinaryOp::Div) op = flir::AluOp::Div;
    else if (node.op == BinaryOp::Eq) op = flir::AluOp::Eq;
    else if (node.op == BinaryOp::Ne) op = flir::AluOp::Ne;
    else if (node.op == BinaryOp::Lt) op = flir::AluOp::Lt;
    else if (node.op == BinaryOp::Le) op = flir::AluOp::Le;
    else if (node.op == BinaryOp::Gt) op = flir::AluOp::Gt;
    else if (node.op == BinaryOp::Ge) op = flir::AluOp::Ge;
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

// ── Dummy stubs for unsupported nodes ────────────────────────────────────────

void FLIRGenerator::visit(FunctionDeclNode& node) {
    // Do not generate code for generic template definitions.
    // Only specialized versions (where genericParams is empty) should be generated.
    if (!node.genericParams.empty()) return;

    resetFunctionState();
    
    const Type* funcType = typeChecker_.typeOf(node.symbolId);
    const Type* retType = nullptr;
    if (auto* fTy = dynamic_cast<const FunctionType*>(funcType)) {
        retType = fTy->returnType;
    }
    
    auto func = std::make_unique<flir::Function>(flir::GlobalId{"@" + std::string(node.name)}, retType);
    currentFunction_ = func.get();
    
    for (auto& p : node.params) {
        const Type* pType = typeChecker_.typeOf(p->symbolId);
        func->params.push_back(flir::Param{pType, nextLocal()});
    }
    
    module_->functions.push_back(std::move(func));
    
    startBlock(nextLabel("entry"));
    
    for (size_t i = 0; i < node.params.size(); ++i) {
        auto& p = node.params[i];
        const Type* pType = typeChecker_.typeOf(p->symbolId);
        flir::LocalId ptr = nextLocal();
        currentBlock_->instructions.push_back(std::make_unique<flir::AllocaInst>(ptr, pType));
        varAllocas_[p->symbolId] = ptr;
        
        flir::LocalId argId = currentFunction_->params[i].id;
        currentBlock_->instructions.push_back(std::make_unique<flir::StoreInst>(argId, ptr));
    }
    
    if (node.body) {
        node.body->accept(*this);
    }
    
    terminateBlock(std::make_unique<flir::RetTerm>());
    currentFunction_ = nullptr;
    currentBlock_ = nullptr;
}
void FLIRGenerator::visit(ParamDeclNode&) {}
void FLIRGenerator::visit(StructDeclNode& node) {
    const Type* stType = typeChecker_.typeOf(node.symbolId);
    if (auto st = dynamic_cast<const StructType*>(stType)) {
        flir::TypeDecl decl;
        decl.name = "%" + std::string(node.name);
        for (auto& field : node.fields) { decl.fields.push_back(typeChecker_.typeOf(field->symbolId)); }
        module_->typeDecls.push_back(decl);
    }
}
void FLIRGenerator::visit(StructFieldNode&) {}
void FLIRGenerator::visit(EnumDeclNode&) {}
void FLIRGenerator::visit(EnumVariantNode&) {}
void FLIRGenerator::visit(TraitDeclNode&) {}
void FLIRGenerator::visit(ImplDeclNode& node) {
    if (!node.genericParams.empty()) return; // Skip generic impls
    for (auto& method : node.methods) {
        method->accept(*this);
    }
}
void FLIRGenerator::visit(ModDeclNode& node) {
    for (auto& d : node.decls) {
        d->accept(*this);
    }
}
void FLIRGenerator::visit(UseDeclNode&) {}
void FLIRGenerator::visit(ExternDeclNode& node) {
    if (node.func) {
        flir::ExternFunction ext;
        ext.name = flir::GlobalId{"@" + std::string(node.func->name)};
        const Type* funcType = typeChecker_.typeOf(node.func->symbolId);
        if (auto* fTy = dynamic_cast<const FunctionType*>(funcType)) {
            ext.paramTypes = fTy->paramTypes;
            ext.returnType = fTy->returnType;
            ext.isVariadic = fTy->isVariadic;
        } else {
            ext.returnType = typeChecker_.getContext().getPrimitive(BuiltinKind::Void);
            ext.isVariadic = node.func->isVariadic;
        }
        module_->externFunctions.push_back(std::move(ext));
    }
}
void FLIRGenerator::visit(TypeAliasDeclNode&) {}

void FLIRGenerator::visit(ExprStmtNode& node) {
    evaluateRValue(*node.expr);
}

void FLIRGenerator::visit(ReturnStmtNode& node) {
    if (node.value) {
        flir::Operand val = evaluateRValue(*node.value);
        terminateBlock(std::make_unique<flir::RetTerm>(val));
    } else {
        terminateBlock(std::make_unique<flir::RetTerm>());
    }
}
void FLIRGenerator::visit(ForStmtNode&) {}
void FLIRGenerator::visit(BreakStmtNode&) {}
void FLIRGenerator::visit(ContinueStmtNode&) {}
void FLIRGenerator::visit(UnsafeStmtNode&) {}
void FLIRGenerator::visit(ComptimeStmtNode&) {}

void FLIRGenerator::visit(UnaryExpr& node) {
    if (node.op == UnaryOp::Ref || node.op == UnaryOp::RefMut) {
        auto oldMode = evalMode_;
        evalMode_ = EvalMode::LValue;
        node.operand->accept(*this);
        evalMode_ = oldMode;

        flir::Operand addr = lastEvaluatedOperand_;
        flir::LocalId dest = nextLocal();
        currentBlock_->instructions.push_back(std::make_unique<flir::BorrowInst>(
            dest, node.op == UnaryOp::RefMut, addr
        ));
        lastEvaluatedOperand_ = flir::Operand(dest);
    } 
    else if (node.op == UnaryOp::Deref) {
        auto oldMode = evalMode_;
        evalMode_ = EvalMode::RValue;
        node.operand->accept(*this);
        evalMode_ = oldMode;

        flir::Operand ptrVal = lastEvaluatedOperand_;

        if (evalMode_ == EvalMode::LValue) {
            lastEvaluatedOperand_ = ptrVal;
        } else {
            flir::LocalId val = nextLocal();
            currentBlock_->instructions.push_back(std::make_unique<flir::LoadInst>(val, std::get<flir::LocalId>(ptrVal)));
            lastEvaluatedOperand_ = flir::Operand(val);
        }
    }
    // Negation and BitNot could be added here later
}

void FLIRGenerator::visit(CallExpr& node) {
    std::vector<flir::Operand> args;
    for (auto& arg : node.args) {
        args.push_back(evaluateRValue(*arg.value));
    }
    
    // Evaluate callee. Prefer resolvedFn if available!
    flir::Operand callee;
    if (node.resolvedFn != kInvalidSymbolID) {
        const auto& sym = table_.getSymbol(node.resolvedFn);
        callee = flir::GlobalId{"@" + sym.name.str()};
    } else if (auto* ident = dynamic_cast<IdentifierExpr*>(node.callee.get())) {
        if (!ident->segments.empty()) {
            callee = flir::GlobalId{"@" + std::string(ident->segments.back())};
        }
    } else {
        callee = evaluateRValue(*node.callee);
    }

    std::optional<flir::LocalId> dest = std::nullopt;
    if (evalMode_ == EvalMode::RValue) {
        dest = nextLocal();
        lastEvaluatedOperand_ = *dest;
    }
    
    currentBlock_->instructions.push_back(
        std::make_unique<flir::CallInst>(dest, callee, args)
    );
}

void FLIRGenerator::visit(MethodCallExpr& node) {
    std::vector<flir::Operand> args;
    args.push_back(evaluateRValue(*node.object)); // Receiver is the first argument
    for (auto& arg : node.args) {
        args.push_back(evaluateRValue(*arg.value));
    }
    
    // The callee must have been resolved by TypeResolver
    flir::Operand callee;
    if (node.resolvedFn != kInvalidSymbolID) {
        const auto& sym = table_.getSymbol(node.resolvedFn);
        callee = flir::GlobalId{"@" + sym.name.str()};
    } else {
        // Fallback for unresolved
        callee = flir::GlobalId{"@" + std::string(node.methodName)};
    }
    
    std::optional<flir::LocalId> dest = std::nullopt;
    if (evalMode_ == EvalMode::RValue) {
        dest = nextLocal();
        lastEvaluatedOperand_ = *dest;
    }
    
    currentBlock_->instructions.push_back(
        std::make_unique<flir::CallInst>(dest, callee, args)
    );
}
void FLIRGenerator::visit(IndexExpr&) {}
void FLIRGenerator::visit(MemberExpr& node) {
    auto oldMode = evalMode_;
    evalMode_ = EvalMode::LValue;
    node.object->accept(*this);
    evalMode_ = oldMode;

    flir::Operand objPtr = lastEvaluatedOperand_; // this is the base pointer
    flir::LocalId fieldPtr = nextLocal();
    std::vector<flir::Operand> offsets = { flir::Number{"0"}, flir::Number{std::to_string(node.resolvedFieldIndex)} };
    currentBlock_->instructions.push_back(std::make_unique<flir::GetPtrInst>(
        fieldPtr, objPtr, offsets
    ));

    if (evalMode_ == EvalMode::LValue) {
        lastEvaluatedOperand_ = flir::Operand(fieldPtr);
    } else {
        flir::LocalId val = nextLocal();
        currentBlock_->instructions.push_back(std::make_unique<flir::LoadInst>(val, fieldPtr));
        lastEvaluatedOperand_ = flir::Operand(val);
    }
}
void FLIRGenerator::visit(CastExpr&) {}
void FLIRGenerator::visit(ArrayLiteralExpr&) {}
void FLIRGenerator::visit(TupleLiteralExpr&) {}
void FLIRGenerator::visit(StructInitExpr& node) {
    auto st = dynamic_cast<const StructType*>(node.inferredType);
    if (!st) return;

    flir::LocalId ptr = nextLocal();
    currentBlock_->instructions.push_back(std::make_unique<flir::AllocaInst>(ptr, st));

    for (auto& field : node.fields) {
        flir::Operand val = evaluateRValue(*field.value);
        auto it = st->fieldIndices.find(std::string(field.name));
        if (it != st->fieldIndices.end()) {
            size_t idx = it->second;
            flir::LocalId fieldPtr = nextLocal();
            currentBlock_->instructions.push_back(std::make_unique<flir::GetPtrInst>(
                fieldPtr, ptr, std::vector<flir::Operand>{flir::Number{"0"}, flir::Number{std::to_string(idx)}}
            ));
            currentBlock_->instructions.push_back(std::make_unique<flir::StoreInst>(val, fieldPtr));
        }
    }

    flir::LocalId res = nextLocal();
    currentBlock_->instructions.push_back(std::make_unique<flir::LoadInst>(res, ptr));
    lastEvaluatedOperand_ = res;
}
void FLIRGenerator::visit(LambdaExpr&) {}
void FLIRGenerator::visit(MatchExpr& node) {
    flir::Operand subj = evaluateRValue(*node.subject);
    
    flir::LabelId endLbl = nextLabel("match_end");
    flir::LabelId defaultLbl = endLbl;
    
    std::vector<std::pair<flir::Number, flir::LabelId>> cases;
    std::vector<std::pair<flir::LabelId, MatchArmNode*>> armBlocks;
    
    for (auto& arm : node.arms) {
        flir::LabelId armLbl = nextLabel("match_arm");
        if (auto* litPat = dynamic_cast<LiteralPatternNode*>(arm.pattern.get())) {
            if (litPat->lit->kind == LiteralKind::Integer) {
                cases.push_back({flir::Number{std::string(litPat->lit->rawText)}, armLbl});
                armBlocks.push_back({armLbl, &arm});
            }
        } else if (dynamic_cast<IdentifierPatternNode*>(arm.pattern.get()) || dynamic_cast<WildcardPatternNode*>(arm.pattern.get())) {
            defaultLbl = armLbl;
            armBlocks.push_back({armLbl, &arm});
        }
    }
    
    terminateBlock(std::make_unique<flir::SwitchTerm>(subj, cases, defaultLbl));
    
    for (auto& armBlock : armBlocks) {
        startBlock(armBlock.first);
        if (armBlock.second->body) {
            armBlock.second->body->accept(*this);
        }
        terminateBlock(std::make_unique<flir::JumpTerm>(endLbl));
    }
    
    startBlock(endLbl);
}
void FLIRGenerator::visit(AwaitExpr&) {}
void FLIRGenerator::visit(SizeofExpr&) {}
void FLIRGenerator::visit(AlignofExpr&) {}

} // namespace fl


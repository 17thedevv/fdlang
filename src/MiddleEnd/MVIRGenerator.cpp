#include "mellis/MiddleEnd/MVIRGenerator.h"
#include "mellis/AST/ASTNode.h"
#include "mellis/AST/ExprNode.h"
#include "mellis/AST/StmtNode.h"
#include "mellis/AST/DeclNode.h"
#include "mellis/AST/ProgramNode.h"
#include "mellis/AST/PatternNode.h"
#include <cassert>
#include <iostream>

namespace fl {

MVIRGenerator::MVIRGenerator(SymbolTable& symTable, TypeChecker& typeChecker)
    : table_(symTable), typeChecker_(typeChecker) {}

std::unique_ptr<mvir::Module> MVIRGenerator::generate(ProgramNode& program) {
    module_ = std::make_unique<mvir::Module>();
    nextLocalId_ = 0;
    nextLabelId_ = 0;
    varAllocas_.clear();
    evalMode_ = EvalMode::RValue;

    visit(program);

    return std::move(module_);
}

// ── Helpers ──────────────────────────────────────────────────────────────────

mvir::LocalId MVIRGenerator::nextLocal() {
    return mvir::LocalId{"%" + std::to_string(nextLocalId_++)};
}

mvir::LabelId MVIRGenerator::nextLabel(const std::string& prefix) {
    return mvir::LabelId{prefix + std::to_string(nextLabelId_++)};
}

void MVIRGenerator::terminateBlock(std::unique_ptr<mvir::Terminator> term) {
    if (currentBlock_ && !currentBlock_->terminator) {
        currentBlock_->terminator = std::move(term);
    }
}

void MVIRGenerator::startBlock(mvir::LabelId label) {
    auto bb = std::make_unique<mvir::BasicBlock>(std::move(label));
    currentBlock_ = bb.get();
    currentFunction_->blocks.push_back(std::move(bb));
}

void MVIRGenerator::resetFunctionState() {
    nextLocalId_ = 0;
    nextLabelId_ = 0;
    varAllocas_.clear();
    currentBlock_ = nullptr;
}

mvir::Operand MVIRGenerator::evaluateRValue(ExprNode& expr) {
    auto oldMode = evalMode_;
    evalMode_ = EvalMode::RValue;
    expr.accept(*this);
    evalMode_ = oldMode;
    return lastEvaluatedOperand_;
}

mvir::Operand MVIRGenerator::evaluateLValue(ExprNode& expr) {
    auto oldMode = evalMode_;
    evalMode_ = EvalMode::LValue;
    expr.accept(*this);
    evalMode_ = oldMode;
    return lastEvaluatedOperand_;
}

// ── ASTVisitor: Statements ───────────────────────────────────────────────────

void MVIRGenerator::visit(ProgramNode& node) {
    for (auto& item : node.items) {
        item->accept(*this);
    }
}

void MVIRGenerator::visit(VarDeclNode& node) {
    // Variable Declaration:
    // 1. Allocate space on stack: %id = alloca type
    // 2. If initialized, evaluate RHS and store: store val, %id

    const Type* varType = typeChecker_.typeOf(node.symbolId);
    
    mvir::LocalId ptr = nextLocal();
    currentBlock_->instructions.push_back(
        std::make_unique<mvir::AllocaInst>(ptr, varType)
    );
    
    // Save pointer location to mapping
    varAllocas_[node.symbolId] = ptr;

    if (!scopeStack_.empty()) {
        scopeStack_.back().push_back(ptr);
    }

    if (node.initializer) {
        mvir::Operand initVal = evaluateRValue(*node.initializer);
        currentBlock_->instructions.push_back(
            std::make_unique<mvir::StoreInst>(initVal, ptr)
        );
    }
}

void MVIRGenerator::visit(AssignExpr& node) {
    // Assignment:
    // 1. Evaluate RHS
    // 2. Load pointer from symbol mapping
    // 3. store val, %id

    mvir::Operand val = evaluateRValue(*node.value);
    mvir::Operand ptr = evaluateLValue(*node.lvalue);

    currentBlock_->instructions.push_back(
        std::make_unique<mvir::StoreInst>(val, ptr)
    );
    
    lastEvaluatedOperand_ = val;
}

void MVIRGenerator::visit(BlockStmtNode& node) {
    currentBlock_->instructions.push_back(std::make_unique<mvir::BeginScopeInst>());
    for (auto& stmt : node.body) {
        stmt->accept(*this);
    }
    currentBlock_->instructions.push_back(std::make_unique<mvir::EndScopeInst>());
}

void MVIRGenerator::visit(IfStmtNode& node) {
    mvir::Operand cond = evaluateRValue(*node.condition);

    mvir::LabelId thenLbl = nextLabel("then");
    mvir::LabelId elseLbl = node.elseBranch ? nextLabel("else") : mvir::LabelId{""};
    mvir::LabelId mergeLbl = nextLabel("merge");

    if (!node.elseBranch) {
        elseLbl = mergeLbl;
    }

    terminateBlock(std::make_unique<mvir::BranchTerm>(cond, thenLbl, elseLbl));

    // Then block
    startBlock(thenLbl);
    node.thenBranch->accept(*this);
    terminateBlock(std::make_unique<mvir::JumpTerm>(mergeLbl));

    // Else block
    if (node.elseBranch) {
        startBlock(elseLbl);
        node.elseBranch->accept(*this);
        terminateBlock(std::make_unique<mvir::JumpTerm>(mergeLbl));
    }

    // Merge block
    startBlock(mergeLbl);
}

void MVIRGenerator::visit(WhileStmtNode& node) {
    mvir::LabelId condLbl = nextLabel("while_cond");
    mvir::LabelId bodyLbl = nextLabel("while_body");
    mvir::LabelId endLbl = nextLabel("while_end");

    terminateBlock(std::make_unique<mvir::JumpTerm>(condLbl));

    // Condition block
    startBlock(condLbl);
    mvir::Operand cond = evaluateRValue(*node.condition);
    terminateBlock(std::make_unique<mvir::BranchTerm>(cond, bodyLbl, endLbl));

    // Body block
    startBlock(bodyLbl);
    node.body->accept(*this);
    terminateBlock(std::make_unique<mvir::JumpTerm>(condLbl));

    // End block
    startBlock(endLbl);
}

// ── ASTVisitor: Expressions ──────────────────────────────────────────────────

void MVIRGenerator::visit(LiteralExpr& node) {
    if (node.kind == LiteralKind::Integer) {
        lastEvaluatedOperand_ = mvir::Number{std::string(node.rawText)};
    } else if (node.kind == LiteralKind::Bool) {
        lastEvaluatedOperand_ = mvir::Boolean{std::string(node.rawText) == "true"};
    } else if (node.kind == LiteralKind::Str) {
        static int stringCounter = 1;
        std::string globalIdStr = "@str_" + std::to_string(stringCounter++);
        mvir::GlobalId gid{globalIdStr};

        mvir::GlobalDecl globalDecl;
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
        lastEvaluatedOperand_ = mvir::Number{"0"}; // Default stub for others
    }
}

void MVIRGenerator::visit(IdentifierExpr& node) {
    if (!varAllocas_.count(node.resolvedSymbol)) {
        std::cerr << "Missing symbolId in varAllocas_: " << node.segments.back() << " (ID: " << node.resolvedSymbol << ")\n";
        assert(false && "Identifier symbolId not allocated");
    }
    mvir::LocalId ptr = varAllocas_[node.resolvedSymbol];

    if (evalMode_ == EvalMode::LValue) {
        // Return pointer directly
        lastEvaluatedOperand_ = ptr;
    } else {
        // Evaluate to value
        mvir::LocalId dest = nextLocal();
        currentBlock_->instructions.push_back(
            std::make_unique<mvir::LoadInst>(dest, ptr)
        );
        lastEvaluatedOperand_ = dest;
    }
}

void MVIRGenerator::visit(BinaryExpr& node) {
    mvir::Operand left = evaluateRValue(*node.left);
    mvir::Operand right = evaluateRValue(*node.right);

    mvir::AluOp op;
    if (node.op == BinaryOp::Add) op = mvir::AluOp::Add;
    else if (node.op == BinaryOp::Sub) op = mvir::AluOp::Sub;
    else if (node.op == BinaryOp::Mul) op = mvir::AluOp::Mul;
    else if (node.op == BinaryOp::Div) op = mvir::AluOp::Div;
    else if (node.op == BinaryOp::Eq) op = mvir::AluOp::Eq;
    else if (node.op == BinaryOp::Ne) op = mvir::AluOp::Ne;
    else if (node.op == BinaryOp::Lt) op = mvir::AluOp::Lt;
    else if (node.op == BinaryOp::Le) op = mvir::AluOp::Le;
    else if (node.op == BinaryOp::Gt) op = mvir::AluOp::Gt;
    else if (node.op == BinaryOp::Ge) op = mvir::AluOp::Ge;
    else {
        assert(false && "Unknown binary operator");
        op = mvir::AluOp::Add;
    }

    mvir::LocalId dest = nextLocal();
    currentBlock_->instructions.push_back(
        std::make_unique<mvir::AluInst>(dest, op, left, right)
    );

    lastEvaluatedOperand_ = dest;
}

// ── Dummy stubs for unsupported nodes ────────────────────────────────────────

void MVIRGenerator::visit(FunctionDeclNode& node) {
    // Do not generate code for generic template definitions.
    // Only specialized versions (where genericParams is empty) should be generated.
    if (!node.genericParams.empty()) return;

    resetFunctionState();
    
    const Type* funcType = typeChecker_.typeOf(node.symbolId);
    const Type* retType = nullptr;
    if (auto* fTy = dynamic_cast<const FunctionType*>(funcType)) {
        retType = fTy->returnType;
    }
    
    auto func = std::make_unique<mvir::Function>(mvir::GlobalId{"@" + std::string(node.name)}, retType);
    currentFunction_ = func.get();
    
    for (auto& p : node.params) {
        const Type* pType = typeChecker_.typeOf(p->symbolId);
        func->params.push_back(mvir::Param{pType, nextLocal()});
    }
    
    module_->functions.push_back(std::move(func));
    
    startBlock(nextLabel("entry"));
    
    for (size_t i = 0; i < node.params.size(); ++i) {
        auto& p = node.params[i];
        const Type* pType = typeChecker_.typeOf(p->symbolId);
        mvir::LocalId ptr = nextLocal();
        currentBlock_->instructions.push_back(std::make_unique<mvir::AllocaInst>(ptr, pType));
        varAllocas_[p->symbolId] = ptr;
        
        mvir::LocalId argId = currentFunction_->params[i].id;
        currentBlock_->instructions.push_back(std::make_unique<mvir::StoreInst>(argId, ptr));
    }
    
    if (node.body) {
        node.body->accept(*this);
    }
    
    terminateBlock(std::make_unique<mvir::RetTerm>());
    currentFunction_ = nullptr;
    currentBlock_ = nullptr;
}
void MVIRGenerator::visit(ParamDeclNode&) {}
void MVIRGenerator::visit(StructDeclNode& node) {
    const Type* stType = typeChecker_.typeOf(node.symbolId);
    if (auto st = dynamic_cast<const StructType*>(stType)) {
        mvir::TypeDecl decl;
        decl.name = "%" + std::string(node.name);
        for (auto& field : node.fields) { decl.fields.push_back(typeChecker_.typeOf(field->symbolId)); }
        module_->typeDecls.push_back(decl);
    }
}
void MVIRGenerator::visit(StructFieldNode&) {}
void MVIRGenerator::visit(EnumDeclNode&) {}
void MVIRGenerator::visit(EnumVariantNode&) {}
void MVIRGenerator::visit(TraitDeclNode&) {}
void MVIRGenerator::visit(ImplDeclNode& node) {
    if (!node.genericParams.empty()) return; // Skip generic impls
    for (auto& method : node.methods) {
        method->accept(*this);
    }
}
void MVIRGenerator::visit(ModDeclNode& node) {
    for (auto& d : node.decls) {
        d->accept(*this);
    }
}
void MVIRGenerator::visit(UseDeclNode&) {}
void MVIRGenerator::visit(ExternDeclNode& node) {
    if (node.func) {
        mvir::ExternFunction ext;
        ext.name = mvir::GlobalId{"@" + std::string(node.func->name)};
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
void MVIRGenerator::visit(TypeAliasDeclNode&) {}

void MVIRGenerator::visit(ExprStmtNode& node) {
    evaluateRValue(*node.expr);
}

void MVIRGenerator::visit(ReturnStmtNode& node) {
    if (node.value) {
        mvir::Operand val = evaluateRValue(*node.value);
        terminateBlock(std::make_unique<mvir::RetTerm>(val));
    } else {
        terminateBlock(std::make_unique<mvir::RetTerm>());
    }
}
void MVIRGenerator::visit(ForStmtNode&) {}
void MVIRGenerator::visit(BreakStmtNode&) {}
void MVIRGenerator::visit(ContinueStmtNode&) {}
void MVIRGenerator::visit(UnsafeStmtNode&) {}
void MVIRGenerator::visit(ComptimeStmtNode&) {}

void MVIRGenerator::visit(UnaryExpr& node) {
    if (node.op == UnaryOp::Ref || node.op == UnaryOp::RefMut) {
        auto oldMode = evalMode_;
        evalMode_ = EvalMode::LValue;
        node.operand->accept(*this);
        evalMode_ = oldMode;

        mvir::Operand addr = lastEvaluatedOperand_;
        mvir::LocalId dest = nextLocal();
        currentBlock_->instructions.push_back(std::make_unique<mvir::BorrowInst>(
            dest, node.op == UnaryOp::RefMut, addr
        ));
        lastEvaluatedOperand_ = mvir::Operand(dest);
    } 
    else if (node.op == UnaryOp::Deref) {
        auto oldMode = evalMode_;
        evalMode_ = EvalMode::RValue;
        node.operand->accept(*this);
        evalMode_ = oldMode;

        mvir::Operand ptrVal = lastEvaluatedOperand_;

        if (evalMode_ == EvalMode::LValue) {
            lastEvaluatedOperand_ = ptrVal;
        } else {
            mvir::LocalId val = nextLocal();
            currentBlock_->instructions.push_back(std::make_unique<mvir::LoadInst>(val, std::get<mvir::LocalId>(ptrVal)));
            lastEvaluatedOperand_ = mvir::Operand(val);
        }
    }
    // Negation and BitNot could be added here later
}

void MVIRGenerator::visit(CallExpr& node) {
    std::vector<mvir::Operand> args;
    for (auto& arg : node.args) {
        args.push_back(evaluateRValue(*arg.value));
    }
    
    // Evaluate callee. Prefer resolvedFn if available!
    mvir::Operand callee;
    if (node.resolvedFn != kInvalidSymbolID) {
        const auto& sym = table_.getSymbol(node.resolvedFn);
        callee = mvir::GlobalId{"@" + sym.name.str()};
    } else if (auto* ident = dynamic_cast<IdentifierExpr*>(node.callee.get())) {
        if (!ident->segments.empty()) {
            callee = mvir::GlobalId{"@" + std::string(ident->segments.back())};
        }
    } else {
        callee = evaluateRValue(*node.callee);
    }

    std::optional<mvir::LocalId> dest = std::nullopt;
    if (evalMode_ == EvalMode::RValue) {
        dest = nextLocal();
        lastEvaluatedOperand_ = *dest;
    }
    
    currentBlock_->instructions.push_back(
        std::make_unique<mvir::CallInst>(dest, callee, args)
    );
}

void MVIRGenerator::visit(MethodCallExpr& node) {
    std::vector<mvir::Operand> args;
    args.push_back(evaluateRValue(*node.object)); // Receiver is the first argument
    for (auto& arg : node.args) {
        args.push_back(evaluateRValue(*arg.value));
    }
    
    // The callee must have been resolved by TypeResolver
    mvir::Operand callee;
    if (node.resolvedFn != kInvalidSymbolID) {
        const auto& sym = table_.getSymbol(node.resolvedFn);
        callee = mvir::GlobalId{"@" + sym.name.str()};
    } else {
        // Fallback for unresolved
        callee = mvir::GlobalId{"@" + std::string(node.methodName)};
    }
    
    std::optional<mvir::LocalId> dest = std::nullopt;
    if (evalMode_ == EvalMode::RValue) {
        dest = nextLocal();
        lastEvaluatedOperand_ = *dest;
    }
    
    currentBlock_->instructions.push_back(
        std::make_unique<mvir::CallInst>(dest, callee, args)
    );
}
void MVIRGenerator::visit(IndexExpr&) {}
void MVIRGenerator::visit(MemberExpr& node) {
    auto oldMode = evalMode_;
    evalMode_ = EvalMode::LValue;
    node.object->accept(*this);
    evalMode_ = oldMode;

    mvir::Operand objPtr = lastEvaluatedOperand_; // this is the base pointer
    mvir::LocalId fieldPtr = nextLocal();
    std::vector<mvir::Operand> offsets = { mvir::Number{"0"}, mvir::Number{std::to_string(node.resolvedFieldIndex)} };
    currentBlock_->instructions.push_back(std::make_unique<mvir::GetPtrInst>(
        fieldPtr, objPtr, offsets
    ));

    if (evalMode_ == EvalMode::LValue) {
        lastEvaluatedOperand_ = mvir::Operand(fieldPtr);
    } else {
        mvir::LocalId val = nextLocal();
        currentBlock_->instructions.push_back(std::make_unique<mvir::LoadInst>(val, fieldPtr));
        lastEvaluatedOperand_ = mvir::Operand(val);
    }
}
void MVIRGenerator::visit(CastExpr&) {}
void MVIRGenerator::visit(ArrayLiteralExpr&) {}
void MVIRGenerator::visit(TupleLiteralExpr&) {}
void MVIRGenerator::visit(StructInitExpr& node) {
    auto st = dynamic_cast<const StructType*>(node.inferredType);
    if (!st) return;

    mvir::LocalId ptr = nextLocal();
    currentBlock_->instructions.push_back(std::make_unique<mvir::AllocaInst>(ptr, st));

    for (auto& field : node.fields) {
        mvir::Operand val = evaluateRValue(*field.value);
        auto it = st->fieldIndices.find(std::string(field.name));
        if (it != st->fieldIndices.end()) {
            size_t idx = it->second;
            mvir::LocalId fieldPtr = nextLocal();
            currentBlock_->instructions.push_back(std::make_unique<mvir::GetPtrInst>(
                fieldPtr, ptr, std::vector<mvir::Operand>{mvir::Number{"0"}, mvir::Number{std::to_string(idx)}}
            ));
            currentBlock_->instructions.push_back(std::make_unique<mvir::StoreInst>(val, fieldPtr));
        }
    }

    mvir::LocalId res = nextLocal();
    currentBlock_->instructions.push_back(std::make_unique<mvir::LoadInst>(res, ptr));
    lastEvaluatedOperand_ = res;
}
void MVIRGenerator::visit(LambdaExpr&) {}
void MVIRGenerator::visit(MatchExpr& node) {
    mvir::Operand subj = evaluateRValue(*node.subject);
    
    mvir::LabelId endLbl = nextLabel("match_end");
    mvir::LabelId defaultLbl = endLbl;
    
    std::vector<std::pair<mvir::Number, mvir::LabelId>> cases;
    std::vector<std::pair<mvir::LabelId, MatchArmNode*>> armBlocks;
    
    for (auto& arm : node.arms) {
        mvir::LabelId armLbl = nextLabel("match_arm");
        if (auto* litPat = dynamic_cast<LiteralPatternNode*>(arm.pattern.get())) {
            if (litPat->lit->kind == LiteralKind::Integer) {
                cases.push_back({mvir::Number{std::string(litPat->lit->rawText)}, armLbl});
                armBlocks.push_back({armLbl, &arm});
            }
        } else if (dynamic_cast<IdentifierPatternNode*>(arm.pattern.get()) || dynamic_cast<WildcardPatternNode*>(arm.pattern.get())) {
            defaultLbl = armLbl;
            armBlocks.push_back({armLbl, &arm});
        }
    }
    
    terminateBlock(std::make_unique<mvir::SwitchTerm>(subj, cases, defaultLbl));
    
    for (auto& armBlock : armBlocks) {
        startBlock(armBlock.first);
        if (armBlock.second->body) {
            armBlock.second->body->accept(*this);
        }
        terminateBlock(std::make_unique<mvir::JumpTerm>(endLbl));
    }
    
    startBlock(endLbl);
}
void MVIRGenerator::visit(AwaitExpr&) {}
void MVIRGenerator::visit(SizeofExpr&) {}
void MVIRGenerator::visit(AlignofExpr&) {}

} // namespace fl


import sys

content = open('src/MiddleEnd/FLIRGenerator.cpp', 'r', encoding='utf-8').read()

content = content.replace('flir::Operand FLIRGenerator::evaluateRValue(ExprNode& expr) {', 'void FLIRGenerator::resetFunctionState() {\n    nextLocalId_ = 0;\n    nextLabelId_ = 0;\n    varAllocas_.clear();\n    currentBlock_ = nullptr;\n}\n\nflir::Operand FLIRGenerator::evaluateRValue(ExprNode& expr) {')

old_prog = '''void FLIRGenerator::visit(ProgramNode& node) {
    // MVP: wrap everything in @main() -> void
    auto mainFunc = std::make_unique<flir::Function>(flir::GlobalId{"@main"}, nullptr);
    currentFunction_ = mainFunc.get();
    module_->functions.push_back(std::move(mainFunc));

    // Start entry block
    startBlock(nextLabel());

    for (auto& item : node.items) {
        item->accept(*this);
    }

    // Terminate the last block of main if not already terminated
    terminateBlock(std::make_unique<flir::RetTerm>());
    
    currentFunction_ = nullptr;
    currentBlock_ = nullptr;
}'''

new_prog = '''void FLIRGenerator::visit(ProgramNode& node) {
    for (auto& item : node.items) {
        item->accept(*this);
    }
}'''
content = content.replace(old_prog, new_prog)

old_func = 'void FLIRGenerator::visit(FunctionDeclNode&) {}'
new_func = '''void FLIRGenerator::visit(FunctionDeclNode& node) {
    resetFunctionState();
    
    auto func = std::make_unique<flir::Function>(flir::GlobalId{"@" + std::string(node.name)}, nullptr);
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
}'''
content = content.replace(old_func, new_func)

# ReturnStmtNode
old_ret = 'void FLIRGenerator::visit(ReturnStmtNode&) {}'
new_ret = '''void FLIRGenerator::visit(ReturnStmtNode& node) {
    if (node.value) {
        flir::Operand val = evaluateRValue(*node.value);
        terminateBlock(std::make_unique<flir::RetTerm>(val));
    } else {
        terminateBlock(std::make_unique<flir::RetTerm>());
    }
}'''
content = content.replace(old_ret, new_ret)

# CallExpr
old_call = '''void FLIRGenerator::visit(CallExpr& node) {
    // If it's print(), we mock it for testing
    if (auto* ident = dynamic_cast<IdentifierExpr*>(node.callee.get())) {
        if (!ident->segments.empty() && ident->segments[0] == "print") {
            flir::Operand val = evaluateRValue(*node.args[0].value);
            std::vector<flir::Operand> args = {val};
            currentBlock_->instructions.push_back(
                std::make_unique<flir::CallInst>(std::nullopt, flir::GlobalId{"@print"}, args)
            );
            return;
        }
    }
}'''

new_call = '''void FLIRGenerator::visit(CallExpr& node) {
    std::vector<flir::Operand> args;
    for (auto& arg : node.args) {
        args.push_back(evaluateRValue(*arg.value));
    }
    
    // Evaluate callee. For MVP, we assume callee is an IdentifierExpr directly pointing to a Function
    flir::Operand callee;
    if (auto* ident = dynamic_cast<IdentifierExpr*>(node.callee.get())) {
        if (!ident->segments.empty()) {
            callee = flir::GlobalId{"@" + std::string(ident->segments.back())};
        }
    } else {
        callee = evaluateRValue(*node.callee);
    }

    const Type* retType = typeChecker_.typeOf(node.astId); // The result type of the call
    std::optional<flir::LocalId> dest = std::nullopt;
    
    // If it returns a value (not void), we allocate a local ID for it
    // Wait, typeChecker_.typeOf(astId) might return unknown or void. 
    // Let's just conservatively assign to a local if we are evaluating an RValue, 
    // or always assign to a local ID.
    if (evalMode_ == EvalMode::RValue) {
        dest = nextLocal();
        lastEvaluatedOperand_ = *dest;
    }
    
    currentBlock_->instructions.push_back(
        std::make_unique<flir::CallInst>(dest, callee, args)
    );
}'''
content = content.replace(old_call, new_call)

with open('src/MiddleEnd/FLIRGenerator.cpp', 'w', encoding='utf-8') as f:
    f.write(content)

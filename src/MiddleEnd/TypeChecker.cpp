#include "mellis/MiddleEnd/TypeChecker.h"
#include "mellis/AST/DeclNode.h"
#include "mellis/AST/StmtNode.h"
#include "mellis/AST/ExprNode.h"
#include "mellis/AST/TypeNode.h"
#include "mellis/AST/PatternNode.h"
#include "mellis/AST/ProgramNode.h"
#include "mellis/MiddleEnd/MonomorphizationEngine.h"
#include <iostream>
#include <typeinfo>
#include <vector>

namespace fl {

enum class ConstraintKind { Equality, Field, MethodCall, Iterable, EnumVariantPattern, Deref };

struct Constraint {
    ConstraintKind kind;
    const Type* expected;
    const Type* actual;
    std::string name;
    std::vector<const Type*> callArgs; // Used for generic args or field types
    std::vector<std::string> callArgNames; // Labels for MethodCall
    PatternNode* pattern = nullptr;
    SourceLocation loc;
    
    Constraint(ConstraintKind k, const Type* exp, const Type* act, std::string n, std::vector<const Type*> args, std::vector<std::string> argNames, SourceLocation l)
        : kind(k), expected(exp), actual(act), name(n), callArgs(args), callArgNames(argNames), loc(l) {}
        
    Constraint(ConstraintKind k, const Type* exp, const Type* act, std::string n, SourceLocation l)
        : kind(k), expected(exp), actual(act), name(n), callArgs({}), loc(l) {}
        
    Constraint(ConstraintKind k, const Type* exp, SymbolID vId, PatternNode* pat, SourceLocation l)
        : kind(k), expected(exp), actual(nullptr), name(std::to_string(vId)), callArgs({}), pattern(pat), loc(l) {}
};

TypeChecker::TypeChecker(SymbolTable& table, DiagnosticEngine& diag, TypeContext& ctx, MonomorphizationEngine* monoEngine)
    : table_(table), diag_(diag), ctx_(ctx), monoEngine_(monoEngine) {}

void TypeChecker::MethodResolver::addMethod(const Type* receiverType, const std::string& name, SymbolID methodId, const FunctionType* type, TypeContext& ctx) {
    printf("[DEBUG MethodResolver::addMethod] receiverType=%p (%s) method=%s methodId=%u\n", (void*)receiverType, receiverType->toString().c_str(), name.c_str(), methodId);
    std::vector<const Type*> newParamTypes;
    for (auto* pt : type->paramTypes) {
        if (auto* gp = dynamic_cast<const GenericParamType*>(pt)) {
            if (gp->name == "Self") {
                newParamTypes.push_back(receiverType);
                continue;
            }
        }
        newParamTypes.push_back(pt);
    }
    const Type* newRetType = type->returnType;
    if (auto* gp = dynamic_cast<const GenericParamType*>(newRetType)) {
        if (gp->name == "Self") newRetType = receiverType;
    }
    const FunctionType* newFnTy = ctx.getFunctionType(type->paramNames, std::move(newParamTypes), newRetType, type->isCallSite, type->isVariadic);
    implMap[receiverType][name] = {methodId, newFnTy};
}

bool TypeChecker::MethodResolver::probe(const Type* receiverType, const std::string& name, MethodInfo& outMethod) {
    printf("[DEBUG MethodResolver::probe] receiverType=%p (%s) method=%s\n", (void*)receiverType, receiverType ? receiverType->toString().c_str() : "null", name.c_str());
    auto it = implMap.find(receiverType);
    if (it != implMap.end()) {
        auto methodIt = it->second.find(name);
        if (methodIt != it->second.end()) {
            outMethod = methodIt->second;
            printf("[DEBUG MethodResolver::probe] FOUND! methodId=%u\n", outMethod.methodId);
            return true;
        }
    }
    printf("[DEBUG MethodResolver::probe] NOT FOUND\n");
    return false;
}

bool TypeChecker::check(ASTNode* root) {
    if (!root) return false;
    typeTable_.resize(table_.symbolCount(), ctx_.getUnknown());

    class TypePrePass : public ASTVisitor, public TypeVisitor {
        SymbolTable& table;
        TypeContext& ctx;
        std::vector<const Type*>& typeTable;
        MethodResolver& methodResolver;
        std::unordered_map<const Type*, std::unordered_set<SymbolID>>& implementedTraits;
        const Type* evaluatedType = nullptr;

    public:
        const Type* evaluateTypeNode(TypeNode* node) {
            if (!node) return ctx.getVoid();
            evaluatedType = nullptr;
            node->accept(static_cast<TypeVisitor&>(*this));
            return evaluatedType ? evaluatedType : ctx.getUnknown();
        }

        TypePrePass(SymbolTable& table, TypeContext& ctx, std::vector<const Type*>& typeTable, MethodResolver& methodResolver, std::unordered_map<const Type*, std::unordered_set<SymbolID>>& implementedTraits)
            : table(table), ctx(ctx), typeTable(typeTable), methodResolver(methodResolver), implementedTraits(implementedTraits) {}

        void visit(ProgramNode& node) override { for (auto& item : node.items) item->accept(*this); }
        void visit(ModDeclNode& node) override { for (auto& d : node.decls) d->accept(*this); }
        void visit(StructDeclNode& node) override {
            const StructType* stType = ctx.getStructType(node.symbolId);
            typeTable[node.symbolId] = stType;
            StructType* mutSt = const_cast<StructType*>(stType);
            for (size_t i = 0; i < node.fields.size(); ++i) {
                auto& field = node.fields[i];
                mutSt->fieldIndices[std::string(field->name)] = i;
                field->symbolId = table.lookup(Identifier(field->name), node.bodyScopeId).value_or(kInvalidSymbolID);
                if (field->symbolId != kInvalidSymbolID) {
                    typeTable[field->symbolId] = evaluateTypeNode(field->type.get());
                }
            }
        }
        void visit(EnumDeclNode& node) override {
            typeTable[node.symbolId] = ctx.getEnumType(node.symbolId);
            for (auto& variant : node.variants) {
                std::vector<const Type*> fieldTypes;
                for (auto& field : variant->fields) {
                    fieldTypes.push_back(evaluateTypeNode(field->type.get()));
                }
                if (variant->symbolId != kInvalidSymbolID) {
                    typeTable[variant->symbolId] = ctx.getTupleType(fieldTypes);
                }
            }
        }
        void visit(FunctionDeclNode& node) override {
            std::vector<std::string> paramNames;
            std::vector<const Type*> paramTypes;
            for (auto& param : node.params) {
                const Type* pt = evaluateTypeNode(param->type.get());
                if (param->symbolId != kInvalidSymbolID) typeTable[param->symbolId] = pt;
                paramNames.push_back(std::string(param->name));
                paramTypes.push_back(pt);
            }
            const Type* retType = evaluateTypeNode(node.returnType.get());
            typeTable[node.symbolId] = ctx.getFunctionType(std::move(paramNames), std::move(paramTypes), retType, false, node.isVariadic);
            
            // Register methods from generic bounds
            for (auto& gp : node.genericParams) {
                if (gp.symbolId != kInvalidSymbolID) {
                    const Type* gpType = ctx.getGenericParamType(gp.symbolId, gp.name);
                    for (auto& bound : gp.bounds) {
                        const Type* boundType = evaluateTypeNode(bound.get());
                        if (auto* traitTy = dynamic_cast<const TraitType*>(boundType)) {
                            implementedTraits[gpType].insert(traitTy->traitId);
                            const auto& traitSym = table.getSymbol(traitTy->traitId);
                            if (traitSym.kind == SymbolKind::Trait && traitSym.decl) {
                                auto* traitDecl = static_cast<TraitDeclNode*>(traitSym.decl);
                                for (auto& m : traitDecl->methods) {
                                    if (m->symbolId != kInvalidSymbolID) {
                                        if (auto* fnTy = dynamic_cast<const FunctionType*>(typeTable[m->symbolId])) {
                                            methodResolver.addMethod(gpType, std::string(m->name), m->symbolId, fnTy, ctx);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        
        void visit(VarDeclNode& node) override {}
        void visit(ParamDeclNode& node) override {}
        void visit(StructFieldNode& node) override {}
        void visit(EnumVariantNode& node) override {}
        void visit(TraitDeclNode& node) override {
            if (node.symbolId != kInvalidSymbolID) {
                typeTable[node.symbolId] = ctx.getTraitType(node.symbolId);
            }
            for (auto& method : node.methods) {
                method->accept(*this);
            }
            
            // Register methods for Generic Params in Trait
            for (auto& gp : node.genericParams) {
                if (gp.symbolId != kInvalidSymbolID) {
                    const Type* gpType = ctx.getGenericParamType(gp.symbolId, gp.name);
                    for (auto& bound : gp.bounds) {
                        const Type* boundType = evaluateTypeNode(bound.get());
                        if (auto* traitTy = dynamic_cast<const TraitType*>(boundType)) {
                            implementedTraits[gpType].insert(traitTy->traitId);
                            const auto& traitSym = table.getSymbol(traitTy->traitId);
                            if (traitSym.kind == SymbolKind::Trait && traitSym.decl) {
                                auto* traitDecl = static_cast<TraitDeclNode*>(traitSym.decl);
                                for (auto& m : traitDecl->methods) {
                                    if (m->symbolId != kInvalidSymbolID) {
                                        if (auto* fnTy = dynamic_cast<const FunctionType*>(typeTable[m->symbolId])) {
                                            methodResolver.addMethod(gpType, std::string(m->name), m->symbolId, fnTy, ctx);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        void visit(ImplDeclNode& node) override {
            const Type* selfType = evaluateTypeNode(node.selfType.get());
            if (!selfType) return;

            auto optSelfId = table.lookup(Identifier(std::string("Self")), node.bodyScopeId);
            if (optSelfId) {
                typeTable[*optSelfId] = selfType;
            }
            
            // Register methods for Generic Params in Impl
            for (auto& gp : node.genericParams) {
                if (gp.symbolId != kInvalidSymbolID) {
                    const Type* gpType = ctx.getGenericParamType(gp.symbolId, gp.name);
                    for (auto& bound : gp.bounds) {
                        const Type* boundType = evaluateTypeNode(bound.get());
                        if (auto* traitTy = dynamic_cast<const TraitType*>(boundType)) {
                            implementedTraits[gpType].insert(traitTy->traitId);
                            const auto& traitSym = table.getSymbol(traitTy->traitId);
                            if (traitSym.kind == SymbolKind::Trait && traitSym.decl) {
                                auto* traitDecl = static_cast<TraitDeclNode*>(traitSym.decl);
                                for (auto& m : traitDecl->methods) {
                                    if (m->symbolId != kInvalidSymbolID) {
                                        if (auto* fnTy = dynamic_cast<const FunctionType*>(typeTable[m->symbolId])) {
                                            methodResolver.addMethod(gpType, std::string(m->name), m->symbolId, fnTy, ctx);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            const TraitType* traitTy = nullptr;
            if (node.traitType) {
                const Type* ty = evaluateTypeNode(node.traitType.get());
                traitTy = dynamic_cast<const TraitType*>(ty);
                if (!traitTy) {
                    // Note: diag needs to be accessible, but TypePrePass doesn't have it.
                    // We might need to pass DiagnosticEngine to TypePrePass.
                }
            }

            for (auto& method : node.methods) {
                method->accept(*this);
                if (method->symbolId != kInvalidSymbolID) {
                    if (auto* fnTy = dynamic_cast<const FunctionType*>(typeTable[method->symbolId])) {
                        methodResolver.addMethod(selfType, std::string(method->name), method->symbolId, fnTy, ctx);
                    }
                }
            }
            
            if (traitTy) {
                implementedTraits[selfType].insert(traitTy->traitId);
                const auto& sym = table.getSymbol(traitTy->traitId);
                if (sym.kind == SymbolKind::Trait && sym.decl) {
                    auto* traitDecl = static_cast<TraitDeclNode*>(sym.decl);
                    // Minimal check: verify all trait methods are implemented
                    // A proper check would unify their types.
                    for (auto& tMethod : traitDecl->methods) {
                        bool found = false;
                        for (auto& iMethod : node.methods) {
                            if (iMethod->name == tMethod->name) {
                                found = true;
                                break;
                            }
                        }
                        if (!found) {
                            // Diagnostics needed
                        }
                    }
                }
            }
        }
        void visit(TypeAliasDeclNode& node) override {}
        void visit(UseDeclNode& node) override {}
        void visit(ExternDeclNode& node) override { if (node.func) node.func->accept(*this); }
        
        void visit(BlockStmtNode& node) override {}
        void visit(ExprStmtNode& node) override {}
        void visit(IfStmtNode& node) override {}
        void visit(WhileStmtNode& node) override {}
        void visit(ForStmtNode& node) override {}
        void visit(ReturnStmtNode& node) override {}
        void visit(BreakStmtNode& node) override {}
        void visit(ContinueStmtNode& node) override {}
        void visit(UnsafeStmtNode& node) override {}
        void visit(ComptimeStmtNode& node) override {}

        void visit(LiteralExpr& node) override {}
        void visit(IdentifierExpr& node) override {}
        void visit(BinaryExpr& node) override {}
        void visit(UnaryExpr& node) override {}
        void visit(AssignExpr& node) override {}
        void visit(CallExpr& node) override {}
        void visit(MethodCallExpr& node) override {}
        void visit(IndexExpr& node) override {}
        void visit(MemberExpr& node) override {}
        void visit(CastExpr& node) override {}
        void visit(ArrayLiteralExpr& node) override {}
        void visit(TupleLiteralExpr& node) override {}
        void visit(StructInitExpr& node) override {}
        void visit(MatchExpr& node) override {}
        void visit(LambdaExpr& node) override {}
        void visit(AwaitExpr& node) override {}
        void visit(SizeofExpr& node) override {}
        void visit(AlignofExpr& node) override {}

        void visit(BuiltinTypeNode& node) override { evaluatedType = ctx.getPrimitive(node.kind); }
        void visit(NamedTypeNode& node) override {
            if (node.symbolId != kInvalidSymbolID) {
                evaluatedType = typeTable[node.symbolId]; 
                if (evaluatedType && evaluatedType->getKind() == TypeKind::Unknown) {
                    const auto& sym = table.getSymbol(node.symbolId);
                    std::vector<const Type*> args;
                    for (auto& argNode : node.genericArgs) {
                        args.push_back(evaluateTypeNode(argNode.get()));
                    }
                    if (sym.kind == SymbolKind::Struct) evaluatedType = ctx.getStructType(node.symbolId, args);
                    else if (sym.kind == SymbolKind::Enum) evaluatedType = ctx.getEnumType(node.symbolId, args);
                    else if (sym.kind == SymbolKind::GenericParam) {
                        // For generic parameters like T, we evaluate them as GenericParamType.
                        evaluatedType = ctx.getGenericParamType(node.symbolId, sym.name.view());
                    }
                }
            } else {
                evaluatedType = ctx.getUnknown();
            }
        }
        void visit(PointerTypeNode& node) override {
            const Type* inner = evaluateTypeNode(node.inner.get());
            evaluatedType = ctx.getPointerType(inner, node.isMutable);
        }
        void visit(ReferenceTypeNode& node) override {
            const Type* inner = evaluateTypeNode(node.inner.get());
            evaluatedType = ctx.getReferenceType(inner, node.isMutable);
        }
        void visit(ArrayTypeNode& node) override {
            const Type* elem = evaluateTypeNode(node.elementType.get());
            // TODO: Evaluate node.size for constant array length. Using 0 for now.
            evaluatedType = ctx.getArrayType(elem, 0); 
        }
        void visit(TupleTypeNode& node) override {
            std::vector<const Type*> elements;
            for (auto& elem : node.elements) {
                elements.push_back(evaluateTypeNode(elem.get()));
            }
            evaluatedType = ctx.getTupleType(std::move(elements));
        }
        void visit(FunctionTypeNode& node) override {
            std::vector<const Type*> paramTypes;
            for (auto& param : node.params) {
                paramTypes.push_back(evaluateTypeNode(param.get()));
            }
            const Type* retType = evaluateTypeNode(node.returnType.get());
            evaluatedType = ctx.getFunctionType(std::move(paramTypes), retType);
        }
        void visit(NeverTypeNode& node) override {
            evaluatedType = ctx.getNever();
        }
        void visit(TraitObjectTypeNode& node) override {
            // Evaluates the trait name to a TraitType
            evaluatedType = evaluateTypeNode(node.trait.get());
        }
    };
    
    class PatternConstraintVisitor : public PatternVisitor {
        TypeContext& ctx;
        std::vector<const Type*>& typeTable;
        std::vector<Constraint>& constraints;
        const Type* expectedType;
        
    public:
        PatternConstraintVisitor(TypeContext& ctx, std::vector<const Type*>& typeTable, std::vector<Constraint>& constraints, const Type* expected)
            : ctx(ctx), typeTable(typeTable), constraints(constraints), expectedType(expected) {}
            
        void visit(WildcardPatternNode& node) override {
            node.inferredType = expectedType;
        }
        
        void visit(LiteralPatternNode& node) override {
            node.inferredType = expectedType;
        }
        
        void visit(IdentifierPatternNode& node) override {
            if (node.symbolId != kInvalidSymbolID) {
                typeTable[node.symbolId] = expectedType;
                node.inferredType = expectedType;
            }
        }
        
        void visit(EnumPatternNode& node) override {
            if (node.variantSymbolId != kInvalidSymbolID) {
                constraints.push_back(Constraint(ConstraintKind::EnumVariantPattern, expectedType, node.variantSymbolId, &node, node.loc));
            }
            node.inferredType = expectedType;
        }
        
        void visit(TuplePatternNode& node) override {
            std::vector<const Type*> elementTypes;
            for (auto& elem : node.elements) {
                const Type* elemVar = ctx.getInferenceVar(ctx.newVar());
                elementTypes.push_back(elemVar);
                PatternConstraintVisitor elemVisitor(ctx, typeTable, constraints, elemVar);
                elem->accept(elemVisitor);
            }
            const Type* tupleType = ctx.getTupleType(elementTypes);
            constraints.push_back(Constraint(ConstraintKind::Equality, tupleType, expectedType, "", node.loc));
            node.inferredType = tupleType;
        }
    };

    class ConstraintGenerator : public ASTVisitor, public TypeVisitor {
        SymbolTable& table;
        TypeContext& ctx;
        std::vector<const Type*>& typeTable;
        std::vector<Constraint>& constraints;
        MethodResolver& methodResolver;
        std::unordered_map<const Type*, std::unordered_set<SymbolID>>& implementedTraits;
        const Type* currentReturnType = nullptr;

        const Type* evaluateTypeNode(TypeNode* node) {
            if (!node) return nullptr;
            TypePrePass pre(table, ctx, typeTable, methodResolver, implementedTraits);
            return pre.evaluateTypeNode(node);
        }
    public:
        ConstraintGenerator(SymbolTable& table, TypeContext& ctx, std::vector<const Type*>& typeTable, std::vector<Constraint>& constraints, MethodResolver& methodResolver, std::unordered_map<const Type*, std::unordered_set<SymbolID>>& implementedTraits)
            : table(table), ctx(ctx), typeTable(typeTable), constraints(constraints), methodResolver(methodResolver), implementedTraits(implementedTraits) {}

        void visit(ProgramNode& node) override { for (auto& item : node.items) item->accept(*this); }
        void visit(ModDeclNode& node) override { for (auto& d : node.decls) d->accept(*this); }
        void visit(ExternDeclNode& node) override { if (node.func) node.func->accept(*this); }
        void visit(VarDeclNode& node) override {
            const Type* annotType = nullptr;
            if (node.typeAnnot) {
                annotType = evaluateTypeNode(node.typeAnnot.get()); 
            }
            
            const Type* varType = annotType ? annotType : ctx.getInferenceVar(ctx.newVar());
            typeTable[node.symbolId] = varType;

            if (node.initializer) {
                node.initializer->accept(*this);
                if (node.initializer->inferredType) {
                    constraints.push_back({ConstraintKind::Equality, varType, node.initializer->inferredType, "", node.loc});
                }
            }
        }
        void visit(ParamDeclNode& node) override {}
        void visit(StructInitExpr& node) override {
            const Type* structType = ctx.getUnknown();
            if (node.structId != kInvalidSymbolID) structType = typeTable[node.structId];
            node.inferredType = structType;
            for (auto& field : node.fields) field.value->accept(*this);
        }
        void visit(FunctionDeclNode& node) override {
            const Type* oldRet = currentReturnType;
            if (auto* fnTy = dynamic_cast<const FunctionType*>(typeTable[node.symbolId])) {
                currentReturnType = fnTy->returnType;
            }
            if (node.body) node.body->accept(*this);
            currentReturnType = oldRet;
        }
        void visit(StructDeclNode& node) override {}
        void visit(StructFieldNode& node) override {}
        void visit(EnumDeclNode& node) override {}
        void visit(EnumVariantNode& node) override {}
        void visit(TraitDeclNode& node) override {}
        void visit(ImplDeclNode& node) override {
            for (auto& method : node.methods) method->accept(*this);
        }
        void visit(TypeAliasDeclNode& node) override {}
        void visit(UseDeclNode& node) override {}
        
        void visit(ExprStmtNode& node) override { node.expr->accept(*this); }
        void visit(BlockStmtNode& node) override { for (auto& s : node.body) s->accept(*this); }
        void visit(IfStmtNode& node) override {
            node.condition->accept(*this);
            if (node.condition->inferredType) {
                constraints.push_back(Constraint(ConstraintKind::Equality, ctx.getPrimitive(BuiltinKind::Bool), node.condition->inferredType, "", node.loc));
            }
            node.thenBranch->accept(*this);
            if (node.elseBranch) node.elseBranch->accept(*this);
        }
        void visit(WhileStmtNode& node) override {
            node.condition->accept(*this);
            if (node.condition->inferredType) {
                constraints.push_back(Constraint(ConstraintKind::Equality, ctx.getPrimitive(BuiltinKind::Bool), node.condition->inferredType, "", node.loc));
            }
            node.body->accept(*this);
        }
        void visit(ForStmtNode& node) override {
            if (node.kind == ForKind::CStyle) {
                if (node.init) node.init->accept(*this);
                if (node.cond) {
                    node.cond->accept(*this);
                    if (node.cond->inferredType) {
                        constraints.push_back(Constraint(ConstraintKind::Equality, ctx.getPrimitive(BuiltinKind::Bool), node.cond->inferredType, "", node.loc));
                    }
                }
                if (node.step) node.step->accept(*this);
            } else {
                node.iterable->accept(*this);
                const Type* elemType = ctx.getInferenceVar(ctx.newVar());
                constraints.push_back(Constraint(ConstraintKind::Iterable, node.iterable->inferredType, elemType, "", node.loc));
                
                if (node.bindingId != kInvalidSymbolID) {
                    typeTable[node.bindingId] = elemType;
                }
            }
            if (node.body) node.body->accept(*this);
        }
        void visit(ReturnStmtNode& node) override {
            if (node.value) {
                node.value->accept(*this);
                if (currentReturnType && node.value->inferredType) {
                    constraints.push_back({ConstraintKind::Equality, currentReturnType, node.value->inferredType, "", node.loc});
                }
            }
        }
        void visit(BreakStmtNode& node) override {}
        void visit(ContinueStmtNode& node) override {}
        void visit(UnsafeStmtNode& node) override {}
        void visit(ComptimeStmtNode& node) override {}

        void visit(LiteralExpr& node) override {
            switch (node.kind) {
                case LiteralKind::Integer: node.inferredType = ctx.getPrimitive(BuiltinKind::I32); break;
                case LiteralKind::Float:   node.inferredType = ctx.getPrimitive(BuiltinKind::F32); break;
                case LiteralKind::Bool:    node.inferredType = ctx.getPrimitive(BuiltinKind::Bool); break;
                case LiteralKind::Str:     node.inferredType = ctx.getPrimitive(BuiltinKind::Str); break;
                case LiteralKind::Char:    node.inferredType = ctx.getPrimitive(BuiltinKind::Char); break;
                default:                   node.inferredType = ctx.getUnknown(); break;
            }
        }
        void visit(IdentifierExpr& node) override {
            if (node.resolvedSymbol != kInvalidSymbolID) {
                node.inferredType = typeTable[node.resolvedSymbol];
            } else {
                node.inferredType = ctx.getUnknown();
            }
        }
        void visit(BinaryExpr& node) override {
            node.left->accept(*this);
            node.right->accept(*this);
            if (node.left->inferredType && node.right->inferredType) {
                constraints.push_back({ConstraintKind::Equality, node.left->inferredType, node.right->inferredType, "", node.loc});
            }
            if (node.op == BinaryOp::Eq || node.op == BinaryOp::Ne || 
                node.op == BinaryOp::Lt || node.op == BinaryOp::Le || 
                node.op == BinaryOp::Gt || node.op == BinaryOp::Ge ||
                node.op == BinaryOp::LogicAnd || node.op == BinaryOp::LogicOr) {
                node.inferredType = ctx.getPrimitive(BuiltinKind::Bool);
            } else {
                node.inferredType = node.left->inferredType ? node.left->inferredType : ctx.getUnknown();
            }
        }
        void visit(UnaryExpr& node) override {
            node.operand->accept(*this);
            node.inferredType = ctx.getInferenceVar(ctx.newVar());
            if (node.op == UnaryOp::Ref || node.op == UnaryOp::RefMut) {
                bool isMut = (node.op == UnaryOp::RefMut);
                const Type* expected = ctx.getReferenceType(node.operand->inferredType, isMut);
                constraints.push_back(Constraint(ConstraintKind::Equality, node.inferredType, expected, "", node.loc));
            } else if (node.op == UnaryOp::Deref) {
                constraints.push_back(Constraint(ConstraintKind::Deref, node.operand->inferredType, node.inferredType, "", node.loc));
            } else if (node.op == UnaryOp::Not) {
                constraints.push_back(Constraint(ConstraintKind::Equality, ctx.getPrimitive(BuiltinKind::Bool), node.operand->inferredType, "", node.loc));
                constraints.push_back(Constraint(ConstraintKind::Equality, ctx.getPrimitive(BuiltinKind::Bool), node.inferredType, "", node.loc));
            } else if (node.op == UnaryOp::Neg || node.op == UnaryOp::BitNot) {
                // Should be numeric, for now just unify with operand
                constraints.push_back(Constraint(ConstraintKind::Equality, node.inferredType, node.operand->inferredType, "", node.loc));
            }
        }
        void visit(AssignExpr& node) override {
            node.lvalue->accept(*this);
            node.value->accept(*this);
            if (node.lvalue->inferredType && node.value->inferredType) {
                constraints.push_back({ConstraintKind::Equality, node.lvalue->inferredType, node.value->inferredType, "", node.loc});
            }
            node.inferredType = node.lvalue->inferredType ? node.lvalue->inferredType : ctx.getUnknown();
        }
        void visit(CallExpr& node) override {
            node.callee->accept(*this);
            std::vector<std::string> argNames;
            std::vector<const Type*> argTypes;
            for (auto& arg : node.args) {
                argNames.push_back(std::string(arg.label));
                if (arg.value) {
                    arg.value->accept(*this);
                    argTypes.push_back(arg.value->inferredType);
                } else {
                    argTypes.push_back(ctx.getUnknown());
                }
            }
            node.inferredType = ctx.getInferenceVar(ctx.newVar());

            if (auto* ident = dynamic_cast<IdentifierExpr*>(node.callee.get())) {
                if (ident->resolvedSymbol != kInvalidSymbolID) {
                    const auto& sym = table.getSymbol(ident->resolvedSymbol);
                    if (sym.kind == SymbolKind::Function && sym.decl) {
                        auto* fnDecl = static_cast<FunctionDeclNode*>(sym.decl);
                        if (!fnDecl->genericParams.empty()) {
                            if (!node.genericArgs.empty()) {
                                for (auto& argNode : node.genericArgs) {
                                    node.inferredGenericArgs.push_back(evaluateTypeNode(argNode.get()));
                                }
                            } else {
                                for (size_t i = 0; i < fnDecl->genericParams.size(); ++i) {
                                    node.inferredGenericArgs.push_back(ctx.getInferenceVar(ctx.newVar()));
                                }
                            }
                            
                            std::unordered_map<SymbolID, const Type*> substitutionMap;
                            for (size_t i = 0; i < fnDecl->genericParams.size() && i < node.inferredGenericArgs.size(); ++i) {
                                substitutionMap[fnDecl->genericParams[i].symbolId] = node.inferredGenericArgs[i];
                            }
                            node.callee->inferredType = ctx.substitute(node.callee->inferredType, substitutionMap);
                        }
                    }
                }
            }

            const Type* expectedFnType = ctx.getFunctionType(std::move(argNames), std::move(argTypes), node.inferredType, true /* isCallSite */);
            constraints.push_back(Constraint(ConstraintKind::Equality, expectedFnType, node.callee->inferredType, "", node.loc));
        }
        
        void visit(MethodCallExpr& node) override {
            node.object->accept(*this);
            std::vector<std::string> argNames;
            std::vector<const Type*> argTypes;
            for (auto& arg : node.args) {
                argNames.push_back(std::string(arg.label));
                if (arg.value) {
                    arg.value->accept(*this);
                    argTypes.push_back(arg.value->inferredType);
                } else {
                    argTypes.push_back(ctx.getUnknown());
                }
            }
            node.inferredType = ctx.getInferenceVar(ctx.newVar());
            // Note: method call arguments don't include 'self' in the AST arg list, so this is fine.
            constraints.push_back(Constraint(ConstraintKind::MethodCall, node.object->inferredType, node.inferredType, std::string(node.methodName), argTypes, argNames, node.loc));
        }
        void visit(IndexExpr& node) override {
            node.base->accept(*this);
            node.index->accept(*this);
            if (node.index->inferredType) {
                constraints.push_back(Constraint(ConstraintKind::Equality, ctx.getPrimitive(BuiltinKind::I32), node.index->inferredType, "", node.loc));
            }
            node.inferredType = ctx.getInferenceVar(ctx.newVar());
            constraints.push_back(Constraint(ConstraintKind::Iterable, node.base->inferredType, node.inferredType, "", node.loc));
        }
        void visit(MemberExpr& node) override {
            node.object->accept(*this);
            node.inferredType = ctx.getInferenceVar(ctx.newVar());
            constraints.push_back(Constraint(ConstraintKind::Field, node.object->inferredType, node.inferredType, std::string(node.member), node.loc));
        }
        void visit(CastExpr& node) override {}
        void visit(ArrayLiteralExpr& node) override {}
        void visit(TupleLiteralExpr& node) override {}
        void visit(MatchExpr& node) override {
            node.subject->accept(*this);
            node.inferredType = ctx.getInferenceVar(ctx.newVar()); // T_ret
            for (auto& arm : node.arms) {
                if (arm.pattern) {
                    PatternConstraintVisitor patVisitor(ctx, typeTable, constraints, node.subject->inferredType);
                    arm.pattern->accept(patVisitor);
                }
                if (arm.body) {
                    arm.body->accept(*this);
                    if (auto* exprStmt = dynamic_cast<ExprStmtNode*>(arm.body.get())) {
                        constraints.push_back(Constraint(ConstraintKind::Equality, node.inferredType, exprStmt->expr->inferredType, "", node.loc));
                    }
                    // TODO: When BlockStmtNode supports tail expressions, unify arm.body's type with node.inferredType
                }
            }
        }
        void visit(LambdaExpr& node) override {}
        void visit(AwaitExpr& node) override {}
        void visit(SizeofExpr& node) override {}
        void visit(AlignofExpr& node) override {}

        void visit(BuiltinTypeNode& node) override {}
        void visit(NamedTypeNode& node) override {}
        void visit(PointerTypeNode& node) override {}
        void visit(ReferenceTypeNode& node) override {}
        void visit(ArrayTypeNode& node) override {}
        void visit(TupleTypeNode& node) override {}
        void visit(FunctionTypeNode& node) override {}
        void visit(NeverTypeNode& node) override {}
        void visit(TraitObjectTypeNode& node) override {}
    };


    class UnificationEngine {
        SymbolTable& table;
        DiagnosticEngine& diag;
        TypeContext& ctx;
        std::vector<const Type*>& typeTable;
        MethodResolver& methodResolver;
    public:
        UnificationEngine(SymbolTable& table, DiagnosticEngine& diag, TypeContext& ctx, std::vector<const Type*>& typeTable, MethodResolver& methodResolver) 
            : table(table), diag(diag), ctx(ctx), typeTable(typeTable), methodResolver(methodResolver) {}

        bool unify(const Type* t1, const Type* t2, SourceLocation loc) {
            t1 = ctx.unificationTable.deepResolve(t1, ctx);
            t2 = ctx.unificationTable.deepResolve(t2, ctx);
            if (!t1 || !t2) return false;
            if (t1 == t2 || t1->equals(t2)) return true;
            
            auto* inf1 = dynamic_cast<const InferenceVarType*>(t1);
            auto* inf2 = dynamic_cast<const InferenceVarType*>(t2);

            if (inf1) {
                ctx.unificationTable.unify(inf1->varId, t2);
                return true;
            }
            if (inf2) {
                ctx.unificationTable.unify(inf2->varId, t1);
                return true;
            }

            if (auto* fn1 = dynamic_cast<const FunctionType*>(t1)) {
                if (auto* fn2 = dynamic_cast<const FunctionType*>(t2)) {
                    
                    const FunctionType* callSite = nullptr;
                    const FunctionType* defSite = nullptr;
                    
                    if (fn1->isCallSite) { callSite = fn1; defSite = fn2; }
                    else if (fn2->isCallSite) { callSite = fn2; defSite = fn1; }
                    
                    if (callSite && defSite) {
                        bool hasNames = false;
                        for (const auto& n : callSite->paramNames) if (!n.empty()) hasNames = true;
                        
                        if (defSite->isVariadic) {
                            if (callSite->paramTypes.size() < defSite->paramTypes.size()) goto mismatch;
                        } else {
                            if (callSite->paramTypes.size() != defSite->paramTypes.size()) goto mismatch;
                        }

                        if (hasNames) {
                            bool seenNamedArg = false;
                            std::vector<bool> provided(defSite->paramTypes.size(), false);
                            for (size_t i = 0; i < callSite->paramNames.size(); ++i) {
                                if (callSite->paramNames[i].empty()) { // Positional
                                    if (seenNamedArg) {
                                        diag.error(loc, "Positional argument cannot follow named arguments");
                                        return false;
                                    }
                                    if (i < provided.size() && provided[i]) {
                                        diag.error(loc, "Parameter provided multiple times");
                                        return false;
                                    }
                                    if (i < provided.size()) provided[i] = true;
                                    
                                    const Type* defType = (defSite->isVariadic && i >= defSite->paramTypes.size()) ? callSite->paramTypes[i] : defSite->paramTypes[i];
                                    if (!unify(callSite->paramTypes[i], defType, loc)) return false;
                                } else { // Named
                                    seenNamedArg = true;
                                    bool found = false;
                                    for (size_t j = 0; j < defSite->paramNames.size(); ++j) {
                                        if (defSite->paramNames[j] == callSite->paramNames[i]) {
                                            if (provided[j]) {
                                                diag.error(loc, "Parameter '" + callSite->paramNames[i] + "' provided multiple times");
                                                return false;
                                            }
                                            provided[j] = true;
                                            if (!unify(callSite->paramTypes[i], defSite->paramTypes[j], loc)) {
                                                return false;
                                            }
                                            found = true;
                                            break;
                                        }
                                    }
                                    if (!found) {
                                        diag.error(loc, "Named argument '" + callSite->paramNames[i] + "' not found in function signature");
                                        return false;
                                    }
                                }
                            }
                        } else {
                            for (size_t i = 0; i < callSite->paramTypes.size(); ++i) {
                                const Type* defType = (defSite->isVariadic && i >= defSite->paramTypes.size()) ? callSite->paramTypes[i] : defSite->paramTypes[i];
                                if (!unify(callSite->paramTypes[i], defType, loc)) return false;
                            }
                        }
                    } else {
                        if (fn1->isVariadic != fn2->isVariadic || fn1->paramTypes.size() != fn2->paramTypes.size()) goto mismatch;
                        for (size_t i = 0; i < fn1->paramTypes.size(); ++i) {
                            if (!unify(fn1->paramTypes[i], fn2->paramTypes[i], loc)) return false;
                        }
                    }
                    return unify(fn1->returnType, fn2->returnType, loc);
                }
            }
            if (auto* prim1 = dynamic_cast<const PrimitiveType*>(t1)) {
                if (prim1->builtinKind == BuiltinKind::Str) {
                    if (auto* ptr2 = dynamic_cast<const PointerType*>(t2)) {
                        if (auto* prim2 = dynamic_cast<const PrimitiveType*>(ptr2->pointee)) {
                            if (prim2->builtinKind == BuiltinKind::I8) return true;
                        }
                    }
                }
            }
            if (auto* ptr1 = dynamic_cast<const PointerType*>(t1)) {
                if (auto* prim2 = dynamic_cast<const PrimitiveType*>(t2)) {
                    if (prim2->builtinKind == BuiltinKind::Str) {
                        if (auto* prim1 = dynamic_cast<const PrimitiveType*>(ptr1->pointee)) {
                            if (prim1->builtinKind == BuiltinKind::I8) return true;
                        }
                    }
                }
                if (auto* ptr2 = dynamic_cast<const PointerType*>(t2)) {
                    if (ptr1->isMutable != ptr2->isMutable) goto mismatch;
                    return unify(ptr1->pointee, ptr2->pointee, loc);
                }
            }

            if (auto* ref1 = dynamic_cast<const ReferenceType*>(t1)) {
                if (auto* ref2 = dynamic_cast<const ReferenceType*>(t2)) {
                    if (ref1->isMutable != ref2->isMutable) goto mismatch;
                    return unify(ref1->pointee, ref2->pointee, loc);
                }
            }
            if (auto* st1 = dynamic_cast<const StructType*>(t1)) {
                if (auto* st2 = dynamic_cast<const StructType*>(t2)) {
                    if (st1->structSymbolId != st2->structSymbolId) goto mismatch;
                    if (st1->genericArgs.size() != st2->genericArgs.size()) goto mismatch;
                    for (size_t i = 0; i < st1->genericArgs.size(); ++i) {
                        if (!unify(st1->genericArgs[i], st2->genericArgs[i], loc)) return false;
                    }
                    return true;
                }
            }

        mismatch:
            diag.error(loc, "Type mismatch: expected '" + t1->toString() + "', found '" + t2->toString() + "'");
            return false;
        }

        void solve(std::vector<Constraint> constraints) {
            bool changed = true;
            int iterations = 0;
            while (changed && iterations++ < 10) {
                changed = false;
                for (const auto& c : constraints) {
                    if (c.kind == ConstraintKind::Equality) {
                        if (unify(c.expected, c.actual, c.loc)) changed = true;
                    } else if (c.kind == ConstraintKind::Field) {
                        const Type* objType = ctx.unificationTable.deepResolve(c.expected, ctx);
                        if (auto* st = dynamic_cast<const StructType*>(objType)) {
                            const auto& sym = table.getSymbol(st->structSymbolId);
                            if (sym.kind == SymbolKind::Struct && sym.decl) {
                                auto* structDecl = static_cast<StructDeclNode*>(sym.decl);
                                for (auto& field : structDecl->fields) {
                                    if (field->name == c.name) {
                                        if (field->symbolId != kInvalidSymbolID) {
                                            if (unify(c.actual, typeTable[field->symbolId], c.loc)) changed = true;
                                        }
                                    }
                                }
                            }
                        }
                    } else if (c.kind == ConstraintKind::MethodCall) {
                        const Type* objType = ctx.unificationTable.deepResolve(c.expected, ctx);
                        if (objType->getKind() == TypeKind::InferenceVar) {
                            // Wait for the object type to be inferred
                        } else {
                            MethodInfo mInfo;
                            if (methodResolver.probe(objType, c.name, mInfo)) {
                                // 1. Construct the synthetic call site function type.
                                // The receiver is inherently positional and prepended to the user's call args.
                                std::vector<std::string> callArgNames;
                                callArgNames.push_back(""); // self has no label at call site (it's the receiver)
                                for (auto& name : c.callArgNames) callArgNames.push_back(name);
                                
                                std::vector<const Type*> callArgTypes;
                                callArgTypes.push_back(objType); // Receiver type (to be checked against param 0)
                                for (auto& argTy : c.callArgs) callArgTypes.push_back(argTy);
                                
                                const Type* expectedFnType = ctx.getFunctionType(std::move(callArgNames), std::move(callArgTypes), c.actual, true /* isCallSite */);
                                
                                // 2. Unify the synthetic call site with the method's definition signature
                                if (unify(expectedFnType, mInfo.type, c.loc)) {
                                    changed = true;
                                }
                            } else {
                                diag.error(c.loc, "No method named '" + c.name + "' found for type '" + objType->toString() + "'");
                                changed = true; // Error reported, don't loop forever
                            }
                        }
                    } else if (c.kind == ConstraintKind::Iterable) {
                        const Type* t1 = ctx.unificationTable.deepResolve(c.expected, ctx); // The iterable
                        const Type* t2 = ctx.unificationTable.deepResolve(c.actual, ctx);   // The element type (inference var)
                        
                        if (t1->getKind() == TypeKind::InferenceVar) {
                            // Wait for iterable to be inferred
                        } else if (auto* arr = dynamic_cast<const ArrayType*>(t1)) {
                            if (unify(arr->elementType, t2, c.loc)) changed = true;
                        } else if (auto* sl = dynamic_cast<const SliceType*>(t1)) {
                            if (unify(sl->elementType, t2, c.loc)) changed = true;
                        } else if (t1->getKind() != TypeKind::Unknown) {
                            diag.error(c.loc, "Type '" + t1->toString() + "' is not iterable");
                        }
                    } else if (c.kind == ConstraintKind::EnumVariantPattern) {
                        const Type* t1 = ctx.unificationTable.deepResolve(c.expected, ctx);
                        if (t1->getKind() == TypeKind::InferenceVar) {
                            // Wait for the subject type to be inferred
                        } else if (auto* enumType = dynamic_cast<const EnumType*>(t1)) {
                            SymbolID variantId = std::stoull(c.name);
                            const auto& sym = table.getSymbol(variantId);
                            
                            // Check if the variant belongs to t1's enum
                            const auto& t1Sym = table.getSymbol(enumType->enumSymbolId);
                            if (t1Sym.kind == SymbolKind::Enum && t1Sym.decl) {
                                auto* t1EnumDecl = static_cast<EnumDeclNode*>(t1Sym.decl);
                                auto lookupResult = table.lookup(Identifier(sym.name), t1EnumDecl->bodyScopeId);
                                if (lookupResult && lookupResult.value() == variantId) {
                                    auto* patNode = static_cast<EnumPatternNode*>(c.pattern);
                                    
                                    // Get variant's expected tuple type from typeTable
                                    if (auto* variantTuple = dynamic_cast<const TupleType*>(typeTable[variantId])) {
                                        if (patNode->fields.size() != variantTuple->elements.size()) {
                                            diag.error(c.loc, "Variant '" + sym.name.str() + "' requires " + std::to_string(variantTuple->elements.size()) + " fields, but " + std::to_string(patNode->fields.size()) + " were provided");
                                        } else {
                                            for (size_t i = 0; i < patNode->fields.size(); ++i) {
                                                const Type* fieldExpected = variantTuple->elements[i];
                                                PatternConstraintVisitor fieldVisitor(ctx, typeTable, constraints, fieldExpected);
                                                patNode->fields[i]->accept(fieldVisitor);
                                            }
                                        }
                                    }
                                    changed = true;
                                } else {
                                    diag.error(c.loc, "Mismatched enum type: expected a variant of '" + t1Sym.name.str() + "'");
                                    changed = true;
                                }
                            }
                        } else if (t1->getKind() != TypeKind::Unknown) {
                            diag.error(c.loc, "Expected enum type for variant pattern, found '" + t1->toString() + "'");
                            changed = true;
                        }
                    } else if (c.kind == ConstraintKind::Deref) {
                        const Type* ptrType = ctx.unificationTable.deepResolve(c.expected, ctx);
                        const Type* valType = ctx.unificationTable.deepResolve(c.actual, ctx);
                        if (ptrType->getKind() == TypeKind::InferenceVar) {
                            // Wait
                        } else if (auto* ptr = dynamic_cast<const PointerType*>(ptrType)) {
                            if (unify(ptr->pointee, valType, c.loc)) changed = true;
                        } else if (auto* ref = dynamic_cast<const ReferenceType*>(ptrType)) {
                            if (unify(ref->pointee, valType, c.loc)) changed = true;
                        } else if (ptrType->getKind() != TypeKind::Unknown) {
                            diag.error(c.loc, "Type '" + ptrType->toString() + "' cannot be dereferenced");
                            changed = true;
                        }
                    }
                }
            }
        }
    };
    class TypeResolver : public ASTVisitor {
        SymbolTable& table;
        DiagnosticEngine& diag;
        TypeContext& ctx;
        MonomorphizationEngine* monoEngine;
        MethodResolver& methodResolver;
    public:
        TypeResolver(SymbolTable& table, DiagnosticEngine& diag, TypeContext& ctx, MonomorphizationEngine* monoEngine, MethodResolver& methodResolver) 
            : table(table), diag(diag), ctx(ctx), monoEngine(monoEngine), methodResolver(methodResolver) {}

        void resolve(const Type*& t, SourceLocation loc) {
            if (!t) return;
            t = ctx.unificationTable.deepResolve(t, ctx);
            if (t->getKind() == TypeKind::InferenceVar) {
                diag.error(loc, "Type annotation needed");
            }
        }
        
        // Helper to deeply resolve generic arguments and request specialization
        void resolveGenericsAndSpecialize(CallExpr& node) {
            if (node.inferredGenericArgs.empty()) return;
            
            auto* ident = dynamic_cast<IdentifierExpr*>(node.callee.get());
            if (!ident || ident->resolvedSymbol == kInvalidSymbolID) return;
            
            std::vector<const Type*> concreteArgs;
            for (auto* t : node.inferredGenericArgs) {
                const Type* resolved = ctx.unificationTable.deepResolve(t, ctx);
                if (resolved->getKind() == TypeKind::InferenceVar) {
                    diag.error(node.loc, "Cannot infer type for generic parameter");
                    return;
                }
                concreteArgs.push_back(resolved);
            }
            node.inferredGenericArgs = concreteArgs;
            
            if (monoEngine) {
                const auto& sym = table.getSymbol(ident->resolvedSymbol);
                if (sym.kind == SymbolKind::Function && sym.decl) {
                    auto* fnDecl = static_cast<FunctionDeclNode*>(sym.decl);
                    try {
                        SymbolID specializedId = monoEngine->requestSpecialization(fnDecl, concreteArgs);
                        if (specializedId != kInvalidSymbolID) {
                            ident->resolvedSymbol = specializedId; // Update Call site!
                            const auto& specSym = table.getSymbol(specializedId);
                            ident->segments.back() = specSym.name.str();
                        }
                        node.resolvedFn = specializedId;
                    } catch (const std::exception& e) {
                        diag.error(node.loc, e.what());
                    }
                }
            }
        }

        void visit(ProgramNode& node) override { for (auto& item : node.items) item->accept(*this); }
        void visit(ModDeclNode& node) override { for (auto& d : node.decls) d->accept(*this); }
        void visit(ExternDeclNode& node) override { if (node.func) node.func->accept(*this); }
        void visit(VarDeclNode& node) override {
            if (node.initializer) node.initializer->accept(*this);
        }
        void visit(ParamDeclNode& node) override {}
        void visit(FunctionDeclNode& node) override { if (node.body) node.body->accept(*this); }
        void visit(StructDeclNode& node) override {}
        void visit(StructFieldNode& node) override {}
        void visit(EnumDeclNode& node) override {}
        void visit(EnumVariantNode& node) override {}
        void visit(TraitDeclNode& node) override {}
        void visit(ImplDeclNode& node) override {
            for (auto& method : node.methods) method->accept(*this);
        }
        void visit(TypeAliasDeclNode& node) override {}
        void visit(UseDeclNode& node) override {}

        void visit(BlockStmtNode& node) override { for (auto& s : node.body) s->accept(*this); }
        void visit(ExprStmtNode& node) override { node.expr->accept(*this); }
        void visit(IfStmtNode& node) override {
            node.condition->accept(*this);
            node.thenBranch->accept(*this);
            if (node.elseBranch) node.elseBranch->accept(*this);
        }
        void visit(WhileStmtNode& node) override {
            node.condition->accept(*this);
            node.body->accept(*this);
        }
        void visit(ForStmtNode& node) override {
            if (node.init) node.init->accept(*this);
            if (node.cond) node.cond->accept(*this);
            if (node.step) node.step->accept(*this);
            if (node.iterable) node.iterable->accept(*this);
            if (node.body) node.body->accept(*this);
        }
        void visit(ReturnStmtNode& node) override { if (node.value) node.value->accept(*this); }
        void visit(BreakStmtNode& node) override {}
        void visit(ContinueStmtNode& node) override {}
        void visit(UnsafeStmtNode& node) override {}
        void visit(ComptimeStmtNode& node) override {}

        void visit(LiteralExpr& node) override { resolve(node.inferredType, node.loc); }
        void visit(IdentifierExpr& node) override { resolve(node.inferredType, node.loc); }
        void visit(BinaryExpr& node) override {
            node.left->accept(*this);
            node.right->accept(*this);
            resolve(node.inferredType, node.loc);
        }
        void visit(UnaryExpr& node) override {
            node.operand->accept(*this);
            resolve(node.inferredType, node.loc);
        }
        void visit(AssignExpr& node) override {
            node.lvalue->accept(*this);
            node.value->accept(*this);
            resolve(node.inferredType, node.loc);
        }
        void visit(CallExpr& node) override {
            node.callee->accept(*this);
            for (auto& arg : node.args) {
                if (arg.value) arg.value->accept(*this);
            }
            resolve(node.inferredType, node.loc);
            resolveGenericsAndSpecialize(node);
        }
        void visit(MethodCallExpr& node) override {
            node.object->accept(*this);
            for (auto& arg : node.args) {
                if (arg.value) arg.value->accept(*this);
            }
            resolve(node.inferredType, node.loc);
            
            MethodInfo mInfo;
            if (methodResolver.probe(node.object->inferredType, std::string(node.methodName), mInfo)) {
                node.resolvedFn = mInfo.methodId;
            }
        }
        void visit(IndexExpr& node) override {
            node.base->accept(*this);
            node.index->accept(*this);
            resolve(node.inferredType, node.loc);
        }
        void visit(MemberExpr& node) override {
            node.object->accept(*this);
            resolve(node.inferredType, node.loc);
            
            const Type* objTy = node.object->inferredType;
            if (auto* refTy = dynamic_cast<const ReferenceType*>(objTy)) objTy = refTy->pointee;
            else if (auto* ptrTy = dynamic_cast<const PointerType*>(objTy)) objTy = ptrTy->pointee;
            
            if (auto* st = dynamic_cast<const StructType*>(objTy)) {
                const auto& sym = table.getSymbol(st->structSymbolId);
                if (sym.kind == SymbolKind::Struct && sym.decl) {
                    auto* structDecl = static_cast<StructDeclNode*>(sym.decl);
                    for (size_t i = 0; i < structDecl->fields.size(); ++i) {
                        if (structDecl->fields[i]->name == node.member) {
                            node.resolvedFieldIndex = i;
                            break;
                        }
                    }
                }
            }
        }
        void visit(CastExpr& node) override {}
        void visit(ArrayLiteralExpr& node) override {}
        void visit(TupleLiteralExpr& node) override {}
        void visit(StructInitExpr& node) override {}
        void visit(MatchExpr& node) override {
            node.subject->accept(*this);
            for (auto& arm : node.arms) {
                if (arm.body) arm.body->accept(*this);
            }
            resolve(node.inferredType, node.loc);
        }
        void visit(LambdaExpr& node) override {}
        void visit(AwaitExpr& node) override {}
        void visit(SizeofExpr& node) override {}
        void visit(AlignofExpr& node) override {}
    };

    TypePrePass pre(table_, ctx_, typeTable_, methodResolver_, implementedTraits_);
    root->accept(pre);

    std::vector<Constraint> constraints;
    ConstraintGenerator gen(table_, ctx_, typeTable_, constraints, methodResolver_, implementedTraits_);
    root->accept(gen);

    UnificationEngine solver(table_, diag_, ctx_, typeTable_, methodResolver_);
    solver.solve(constraints);

    for (auto& t : typeTable_) {
        if (t) t = ctx_.unificationTable.deepResolve(t, ctx_);
    }

    TypeResolver resolver(table_, diag_, ctx_, monoEngine_, methodResolver_);
    root->accept(resolver);

    return !diag_.hasErrors();
}


const Type* TypeChecker::typeOf(SymbolID sym) const {
    if (sym < typeTable_.size()) return typeTable_[sym];
    return nullptr;
}

void TypeChecker::registerImpl(const Type* type, SymbolID traitId) {
    if (type) implementedTraits_[type].insert(traitId);
}

bool TypeChecker::implementsTrait(const Type* type, SymbolID traitId) const {
    if (!type) return false;
    auto it = implementedTraits_.find(type);
    if (it != implementedTraits_.end()) {
        return it->second.count(traitId) > 0;
    }
    return false;
}

} // namespace fl

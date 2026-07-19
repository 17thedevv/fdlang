
import re

with open("src/MiddleEnd/MVIRGenerator.cpp", "r") as f:
    content = f.read()

# We want to replace the `void MVIRGenerator::visit(MatchExpr& node)` method body
start_idx = content.find("void MVIRGenerator::visit(MatchExpr& node) {")
if start_idx == -1:
    print("Could not find MatchExpr visitor")
    exit(1)

# Find the matching closing brace
end_idx = start_idx
brace_count = 0
found_open = False
while end_idx < len(content):
    if content[end_idx] == "{":
        brace_count += 1
        found_open = True
    elif content[end_idx] == "}":
        brace_count -= 1
        if found_open and brace_count == 0:
            end_idx += 1
            break
    end_idx += 1

new_method = """void MVIRGenerator::visit(MatchExpr& node) {
    mvir::Operand subj = evaluateRValue(*node.subject);
    mvir::Operand matchSubject = subj;
    
    if (node.subject->inferredType && node.subject->inferredType->getKind() == TypeKind::Enum) {
        mvir::LocalId tagPtr = nextLocal();
        currentBlock_->instructions.push_back(std::make_unique<mvir::TagInst>(tagPtr, subj));
        matchSubject = tagPtr;
    }
    
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
        } else if (auto* enumPat = dynamic_cast<EnumPatternNode*>(arm.pattern.get())) {
            if (enumPat->variantSymbolId != kInvalidSymbolID) {
                // Find variant index
                size_t variantIdx = 0;
                if (auto* enumType = dynamic_cast<const EnumType*>(node.subject->inferredType)) {
                    auto enumSym = table_.getSymbol(enumType->enumSymbolId);
                    if (enumSym.kind == SymbolKind::Enum && enumSym.decl) {
                        auto* enumDecl = static_cast<EnumDeclNode*>(enumSym.decl);
                        for (size_t i = 0; i < enumDecl->variants.size(); ++i) {
                            if (enumDecl->variants[i]->symbolId == enumPat->variantSymbolId) {
                                variantIdx = i;
                                break;
                            }
                        }
                    }
                }
                cases.push_back({mvir::Number{std::to_string(variantIdx)}, armLbl});
                armBlocks.push_back({armLbl, &arm});
            }
        } else if (dynamic_cast<IdentifierPatternNode*>(arm.pattern.get()) || dynamic_cast<WildcardPatternNode*>(arm.pattern.get())) {
            defaultLbl = armLbl;
            armBlocks.push_back({armLbl, &arm});
        }
    }
    
    mvir::LocalId resultPtr = mvir::LocalId{""};
    if (node.inferredType && node.inferredType->getKind() != TypeKind::Unknown) {
        resultPtr = nextLocal();
        currentBlock_->instructions.push_back(std::make_unique<mvir::LocalInst>(resultPtr, node.inferredType));
    }

    terminateBlock(std::make_unique<mvir::SwitchTerm>(matchSubject, cases, defaultLbl));
    
    for (auto& armBlock : armBlocks) {
        startBlock(armBlock.first);
        
        // If it is an enum pattern with fields, extract them!
        if (auto* enumPat = dynamic_cast<EnumPatternNode*>(armBlock.second->pattern.get())) {
            size_t variantIdx = 0;
            if (auto* enumType = dynamic_cast<const EnumType*>(node.subject->inferredType)) {
                auto enumSym = table_.getSymbol(enumType->enumSymbolId);
                if (enumSym.kind == SymbolKind::Enum && enumSym.decl) {
                    auto* enumDecl = static_cast<EnumDeclNode*>(enumSym.decl);
                    for (size_t i = 0; i < enumDecl->variants.size(); ++i) {
                        if (enumDecl->variants[i]->symbolId == enumPat->variantSymbolId) {
                            variantIdx = i;
                            break;
                        }
                    }
                }
            }
            
            for (size_t i = 0; i < enumPat->fields.size(); ++i) {
                if (auto* identPat = dynamic_cast<IdentifierPatternNode*>(enumPat->fields[i].get())) {
                    mvir::LocalId fieldVal = nextLocal();
                    currentBlock_->instructions.push_back(std::make_unique<mvir::ExtractInst>(fieldVal, subj, variantIdx, i));
                    
                    mvir::LocalId fieldPtr = nextLocal();
                    currentBlock_->instructions.push_back(std::make_unique<mvir::LocalInst>(fieldPtr, identPat->inferredType));
                    currentBlock_->instructions.push_back(std::make_unique<mvir::StoreInst>(identPat->inferredType, fieldVal, fieldPtr));
                    
                    varAllocas_[identPat->symbolId] = fieldPtr;
                }
            }
        } else if (auto* identPat = dynamic_cast<IdentifierPatternNode*>(armBlock.second->pattern.get())) {
            mvir::LocalId varPtr = nextLocal();
            currentBlock_->instructions.push_back(std::make_unique<mvir::LocalInst>(varPtr, identPat->inferredType));
            currentBlock_->instructions.push_back(std::make_unique<mvir::StoreInst>(identPat->inferredType, subj, varPtr));
            varAllocas_[identPat->symbolId] = varPtr;
        }
        
        if (armBlock.second->body) {
            armBlock.second->body->accept(*this);
            if (!resultPtr.name.empty() && lastEvaluatedOperand_.index() != 0) {
                currentBlock_->instructions.push_back(std::make_unique<mvir::StoreInst>(node.inferredType, lastEvaluatedOperand_, resultPtr));
            }
        }
        terminateBlock(std::make_unique<mvir::JumpTerm>(endLbl));
    }
    
    startBlock(endLbl);
    if (!resultPtr.name.empty()) {
        mvir::LocalId res = nextLocal();
        currentBlock_->instructions.push_back(std::make_unique<mvir::LoadInst>(res, node.inferredType, resultPtr));
        lastEvaluatedOperand_ = res;
    } else {
        lastEvaluatedOperand_ = mvir::Operand{};
    }
}"""

content = content[:start_idx] + new_method + content[end_idx:]

with open("src/MiddleEnd/MVIRGenerator.cpp", "w") as f:
    f.write(content)

print("Replaced MVIRGenerator::visit(MatchExpr)")

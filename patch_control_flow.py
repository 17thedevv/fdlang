import sys

content = open('src/MiddleEnd/FLIRGenerator.cpp', 'r', encoding='utf-8').read()

# Add include for PatternNode.h
if '#include "fdlang/AST/PatternNode.h"' not in content:
    content = content.replace('#include "fdlang/AST/ProgramNode.h"', '#include "fdlang/AST/ProgramNode.h"\n#include "fdlang/AST/PatternNode.h"')

old_match = 'void FLIRGenerator::visit(MatchExpr&) {}'
new_match = '''void FLIRGenerator::visit(MatchExpr& node) {
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
        } else if (dynamic_cast<IdentifierPatternNode*>(arm.pattern.get())) {
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
}'''

content = content.replace(old_match, new_match)

with open('src/MiddleEnd/FLIRGenerator.cpp', 'w', encoding='utf-8') as f:
    f.write(content)

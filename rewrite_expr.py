
import re

with open("src/MiddleEnd/MVIRGenerator.cpp", "r") as f:
    content = f.read()

# Update IdentifierExpr
id_start = content.find("void MVIRGenerator::visit(IdentifierExpr& node) {")
id_end = content.find("}", id_start) + 1
while content[id_end] != "}":
    id_end = content.find("}", id_end) + 1

new_id_method = """void MVIRGenerator::visit(IdentifierExpr& node) {
    if (node.resolvedSymbol != kInvalidSymbolID) {
        const auto& sym = table_.getSymbol(node.resolvedSymbol);
        if (sym.kind == SymbolKind::EnumVariant) {
            size_t variantIdx = 0;
            const Type* enumTy = node.inferredType;
            if (auto* enumType = dynamic_cast<const EnumType*>(enumTy)) {
                const auto& enumSym = table_.getSymbol(enumType->enumSymbolId);
                if (enumSym.kind == SymbolKind::Enum && enumSym.decl) {
                    auto* enumDecl = static_cast<EnumDeclNode*>(enumSym.decl);
                    for (size_t i = 0; i < enumDecl->variants.size(); ++i) {
                        if (enumDecl->variants[i]->symbolId == node.resolvedSymbol) {
                            variantIdx = i;
                            break;
                        }
                    }
                }
            }
            mvir::LocalId dest = nextLocal();
            currentBlock_->instructions.push_back(std::make_unique<mvir::VariantInst>(dest, enumTy, variantIdx, std::vector<mvir::Operand>{}));
            lastEvaluatedOperand_ = dest;
            return;
        }
    }

    if (!varAllocas_.count(node.resolvedSymbol)) {
        std::cerr << "Missing symbolId in varAllocas_: " << node.segments.back() << " (ID: " << node.resolvedSymbol << ")\\n";
        assert(false && "Identifier symbolId not allocated");
    }
    mvir::LocalId ptr = varAllocas_[node.resolvedSymbol];

    if (evalMode_ == EvalMode::LValue) {
        lastEvaluatedOperand_ = ptr;
    } else {
        mvir::LocalId dest = nextLocal();
        currentBlock_->instructions.push_back(
            std::make_unique<mvir::LoadInst>(dest, node.inferredType, ptr)
        );
        lastEvaluatedOperand_ = dest;
    }
}"""

content = content[:id_start] + new_id_method + content[id_end:]

with open("src/MiddleEnd/MVIRGenerator.cpp", "w") as f:
    f.write(content)

print("Replaced MVIRGenerator::visit(IdentifierExpr)")

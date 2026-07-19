
import re

with open("src/BackEnd/LLVMIRGenerator.cpp", "r") as f:
    content = f.read()

# 1. Map EnumType in mapType
map_type_start = content.find("if (auto* et = dynamic_cast<const EnumType*>(type)) {")
map_type_end = content.find("}", map_type_start) + 1
while content[map_type_end] != "}":
    map_type_end = content.find("}", map_type_end) + 1

new_map_type = """if (auto* et = dynamic_cast<const EnumType*>(type)) {
        std::vector<llvm::Type*> elements;
        elements.push_back(llvm::Type::getInt32Ty(context_)); // Tag
        elements.push_back(llvm::ArrayType::get(llvm::Type::getInt64Ty(context_), 4)); // 32-byte payload padding
        return llvm::StructType::get(context_, elements, false);
    }"""
content = content[:map_type_start] + new_map_type + content[map_type_end:]


# 2. Implement VariantInst, TagInst, ExtractInst in emitInstruction
emit_idx = content.find("else if (auto* castinst = dynamic_cast<const mvir::CastInst*>(inst)) {")
new_instructions = """else if (auto* varInst = dynamic_cast<const mvir::VariantInst*>(inst)) {
        llvm::Type* enumTy = mapType(varInst->type);
        llvm::Value* ptr = builder_.CreateAlloca(enumTy);
        
        llvm::Value* tagPtr = builder_.CreateStructGEP(enumTy, ptr, 0);
        builder_.CreateStore(builder_.getInt32(varInst->tag), tagPtr);
        
        if (!varInst->args.empty()) {
            std::vector<llvm::Type*> variantFields;
            variantFields.push_back(llvm::Type::getInt32Ty(context_));
            for (const auto& arg : varInst->args) {
                variantFields.push_back(mapOperand(arg)->getType());
            }
            llvm::Type* variantTy = llvm::StructType::get(context_, variantFields, false);
            for (size_t i = 0; i < varInst->args.size(); ++i) {
                llvm::Value* argVal = mapOperand(varInst->args[i]);
                llvm::Value* fieldPtr = builder_.CreateStructGEP(variantTy, ptr, i + 1);
                builder_.CreateStore(argVal, fieldPtr);
            }
        }
        
        llvm::Value* enumVal = builder_.CreateLoad(enumTy, ptr);
        localValues_[varInst->dest.name] = enumVal;
    }
    else if (auto* tagInst = dynamic_cast<const mvir::TagInst*>(inst)) {
        llvm::Value* subj = mapOperand(tagInst->subject);
        llvm::Value* tagVal = builder_.CreateExtractValue(subj, {0}, tagInst->dest.name.substr(1));
        localValues_[tagInst->dest.name] = tagVal;
    }
    else if (auto* extractInst = dynamic_cast<const mvir::ExtractInst*>(inst)) {
        llvm::Value* subj = mapOperand(extractInst->subject);
        llvm::Type* enumTy = subj->getType();
        llvm::Value* ptr = builder_.CreateAlloca(enumTy);
        builder_.CreateStore(subj, ptr);
        
        // Wait, how do we know the types of the payload fields?
        // In LLVMIRGenerator, we don't have AST access easily here.
        // We will just let TypeChecker populate it! But ExtractInst in MVIR doesn't have field types.
        // Let's fix MVIR TagInst and ExtractInst in MVIR.h next.
        // Actually, ExtractInst can be implemented if it carries the expected Type.
        // For now we will just use it.
    }
    """
content = content[:emit_idx] + new_instructions + content[emit_idx:]

with open("src/BackEnd/LLVMIRGenerator.cpp", "w") as f:
    f.write(content)

print("Rewritten LLVMIRGenerator!")

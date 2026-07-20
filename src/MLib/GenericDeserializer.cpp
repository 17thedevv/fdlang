#include "mellis/MLib/GenericDeserializer.h"

namespace fl {
namespace mlib {

GenericDeserializer::GenericDeserializer(const uint8_t* data, size_t size)
    : sectionData(data), sectionSize(size) {}

void GenericDeserializer::parseIndexTable() {
    BinaryReader reader(sectionData, sectionSize);
    
    // Read Section Version
    uint32_t version = reader.readU32();
    if (version != 1) {
        throw std::runtime_error("Unsupported Generic Section Version");
    }

    uint32_t count = reader.readU32();
    for (uint32_t i = 0; i < count; ++i) {
        GenericFunctionIndex idx;
        reader.readStruct(idx);
        
        IndexEntry entry;
        entry.offset = idx.offset;
        entry.compressedSize = idx.compressedSize;
        entry.uncompressedSize = idx.uncompressedSize;
        
        indexTable[idx.functionID] = entry;
    }
}

mvir::Operand GenericDeserializer::deserializeOperand(BinaryReader& reader) {
    uint8_t tag = reader.readU8();
    
    if (tag == 0) { // LocalId
        return mvir::LocalId{reader.readString()};
    } else if (tag == 1) { // GlobalId
        return mvir::GlobalId{reader.readString()};
    } else if (tag == 2) { // Number
        return mvir::Number{reader.readString()};
    } else if (tag == 3) { // Boolean
        return mvir::Boolean{reader.readU8() != 0};
    }
    throw std::runtime_error("Invalid operand tag");
}

std::unique_ptr<mvir::Instruction> GenericDeserializer::deserializeInstruction(BinaryReader& reader) {
    mvir::Opcode opc = static_cast<mvir::Opcode>(reader.readU8());
    
    if (opc == mvir::Opcode::Alu) {
        std::string dest = reader.readString();
        mvir::AluOp aluOp = static_cast<mvir::AluOp>(reader.readU8());
        auto left = deserializeOperand(reader);
        auto right = deserializeOperand(reader);
        return std::make_unique<mvir::AluInst>(mvir::LocalId{dest}, aluOp, left, right);
    } else if (opc == mvir::Opcode::Load) {
        std::string dest = reader.readString();
        uint32_t dummyTypeID = reader.readU32();
        auto ptr = deserializeOperand(reader);
        // Note: For full implementation, resolve TypeID to const Type* here.
        return std::make_unique<mvir::LoadInst>(mvir::LocalId{dest}, nullptr, ptr);
    }
    
    throw std::runtime_error("Unsupported opcode in deserializer stub");
}

std::unique_ptr<mvir::Terminator> GenericDeserializer::deserializeTerminator(BinaryReader& reader) {
    mvir::Opcode opc = static_cast<mvir::Opcode>(reader.readU8());
    
    if (opc == mvir::Opcode::Ret) {
        uint8_t hasValue = reader.readU8();
        if (hasValue) {
            auto val = deserializeOperand(reader);
            return std::make_unique<mvir::RetTerm>(std::move(val));
        } else {
            return std::make_unique<mvir::RetTerm>();
        }
    }
    
    throw std::runtime_error("Unsupported terminator opcode in deserializer stub");
}

std::unique_ptr<mvir::Function> GenericDeserializer::loadFunction(uint32_t functionID) {
    auto it = indexTable.find(functionID);
    if (it == indexTable.end()) {
        return nullptr;
    }

    BinaryReader reader(sectionData, sectionSize);
    reader.seek(it->second.offset);

    // 1. Read Header
    GenericPackageHeader header;
    reader.readStruct(header);

    // 2. Read Generic Params & Constraints
    for (uint16_t i = 0; i < header.genericCount; ++i) {
        reader.readU32(); // generic type ID
    }
    for (uint16_t i = 0; i < header.constraintCount; ++i) {
        reader.readU32(); // constraint trait ID
    }

    // 3. Construct MVIR Function
    auto func = std::make_unique<mvir::Function>();
    func->name = mvir::GlobalId{"dummy_generic"};
    
    // 4. Read Blocks
    uint32_t blockCount = reader.readU32();
    for (uint32_t i = 0; i < blockCount; ++i) {
        std::string labelName = reader.readString();
        auto block = std::make_unique<mvir::BasicBlock>(mvir::LabelId{labelName});
        
        uint32_t instCount = reader.readU32();
        for (uint32_t j = 0; j < instCount; ++j) {
            block->instructions.push_back(deserializeInstruction(reader));
        }
        
        block->terminator = deserializeTerminator(reader);
        func->blocks.push_back(std::move(block));
    }

    return func;
}

} // namespace mlib
} // namespace fl

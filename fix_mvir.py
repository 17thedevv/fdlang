import re

with open('include/mellis/MiddleEnd/MVIR.h', 'r') as f:
    content = f.read()

content = content.replace('struct AllocaInst', 'struct LocalInst')
content = content.replace('AllocaInst(', 'LocalInst(')

get_ptr_inst = '''struct GetPtrInst : public Instruction {
    LocalId dest;
    Operand base;
    std::vector<Operand> offsets;

    GetPtrInst(LocalId d, Operand b, std::vector<Operand> offs) : dest(std::move(d)), base(std::move(b)), offsets(std::move(offs)) {}
    std::string toString() const override;
};'''

new_ptr_inst = '''struct IndexInst : public Instruction {
    LocalId dest;
    Operand base;
    Operand index;

    IndexInst(LocalId d, Operand b, Operand idx) : dest(std::move(d)), base(std::move(b)), index(std::move(idx)) {}
    std::string toString() const override;
};

struct FieldInst : public Instruction {
    LocalId dest;
    Operand base;
    size_t fieldIndex;

    FieldInst(LocalId d, Operand b, size_t fIdx) : dest(std::move(d)), base(std::move(b)), fieldIndex(fIdx) {}
    std::string toString() const override;
};'''
content = content.replace(get_ptr_inst, new_ptr_inst)

call_inst = '''struct CallInst : public Instruction {'''
new_call_inst = '''struct ExtractInst : public Instruction {
    LocalId dest;
    Operand base;
    size_t variantIndex;
    size_t fieldIndex;

    ExtractInst(LocalId d, Operand b, size_t vIdx, size_t fIdx)
        : dest(std::move(d)), base(std::move(b)), variantIndex(vIdx), fieldIndex(fIdx) {}
    std::string toString() const override;
};

struct TagInst : public Instruction {
    LocalId dest;
    Operand base;

    TagInst(LocalId d, Operand b) : dest(std::move(d)), base(std::move(b)) {}
    std::string toString() const override;
};

struct VariantInst : public Instruction {
    LocalId dest;
    const Type* enumType;
    size_t variantIndex;
    std::vector<Operand> args;

    VariantInst(LocalId d, const Type* t, size_t vIdx, std::vector<Operand> a)
        : dest(std::move(d)), enumType(t), variantIndex(vIdx), args(std::move(a)) {}
    std::string toString() const override;
};

struct CallInst : public Instruction {'''
content = content.replace(call_inst, new_call_inst)

with open('include/mellis/MiddleEnd/MVIR.h', 'w') as f:
    f.write(content)

print("Done")

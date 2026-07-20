import re

def process_mvir_h():
    with open("include/mellis/MiddleEnd/MVIR.h", "r") as f:
        content = f.read()
    
    # 1. Insert Opcode enum before Instruction
    opcode_enum = """
enum class Opcode : uint8_t {
    // Memory
    Local, Load, Store, Index, Field, Borrow, Cast, Drop, Sizeof, Alignof,
    // ALU
    Alu, Unary,
    // Extract
    Extract, Tag, Variant, MakeTraitObject, Call, VirtualCall, Await,
    // Terminators
    Jump, Branch, Match, Return
};

struct Instruction {
    virtual ~Instruction() = default;
    virtual std::string toString() const = 0;
    virtual Opcode getOpcode() const = 0;
};
"""
    content = re.sub(r'struct Instruction \{\s*virtual ~Instruction\(\) = default;\s*virtual std::string toString\(\) const = 0;\s*\};', opcode_enum, content)

    # 2. Add getOpcode() to all instructions
    def add_opcode(match):
        name = match.group(1)
        return f"struct {name}Inst : public Instruction {{\n    Opcode getOpcode() const override {{ return Opcode::{name}; }}"

    content = re.sub(r'struct (\w+)Inst : public Instruction \{', add_opcode, content)
    
    # 3. Add getOpcode() to Terminators
    term_base = """struct Terminator {
    virtual ~Terminator() = default;
    virtual std::string toString() const = 0;
    virtual Opcode getOpcode() const = 0;
};"""
    content = re.sub(r'struct Terminator \{\s*virtual ~Terminator\(\) = default;\s*virtual std::string toString\(\) const = 0;\s*\};', term_base, content)
    
    def add_term_opcode(match):
        name = match.group(1)
        return f"struct {name}Term : public Terminator {{\n    Opcode getOpcode() const override {{ return Opcode::{name}; }}"
        
    content = re.sub(r'struct (\w+)Term : public Terminator \{', add_term_opcode, content)

    with open("include/mellis/MiddleEnd/MVIR.h", "w") as f:
        f.write(content)
    print("Patched MVIR.h")

if __name__ == "__main__":
    process_mvir_h()

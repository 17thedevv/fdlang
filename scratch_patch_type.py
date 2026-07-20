
import re

with open("include/mellis/Core/FLType.h", "r", encoding="utf-8") as f:
    content = f.read()

content = content.replace("Unknown", "Unknown,\n    Error")

error_type_def = """// -----------------------------------------------------------------------------
// Error Type (Used for Error Recovery)
// -----------------------------------------------------------------------------
class ErrorType : public Type {
public:
    TypeKind getKind() const override { return TypeKind::Error; }
    std::string toString() const override { return "{error}"; }
    bool equals(const Type* other) const override {
        return other->getKind() == TypeKind::Error;
    }
};
"""
content = content.replace("class UnknownType : public Type {", error_type_def + "\nclass UnknownType : public Type {")

content = content.replace("const Type* unknownType_ = nullptr;", "const Type* unknownType_ = nullptr;\n    const Type* errorType_ = nullptr;")

content = content.replace("unknownType_ = create<UnknownType>();", "unknownType_ = create<UnknownType>();\n        errorType_ = create<ErrorType>();")

content = content.replace("const Type* getUnknown() const { return unknownType_; }", "const Type* getUnknown() const { return unknownType_; }\n    const Type* getError() const { return errorType_; }")

content = content.replace("return t; // primitive, unknown, void, never", "return t; // primitive, unknown, void, never, error")

with open("include/mellis/Core/FLType.h", "w", encoding="utf-8") as f:
    f.write(content)

print("Patched FLType.h")


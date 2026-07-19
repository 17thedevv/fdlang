import re

with open('src/MiddleEnd/TypeChecker.cpp', 'r') as f:
    content = f.read()

# Skip in TypePrePass
content = re.sub(
    r"(class TypePrePass.*?void visit\(FunctionDeclNode& node\) override \{)",
    r"\1\n            if (!node.genericParams.empty()) return;",
    content,
    flags=re.DOTALL
)
content = re.sub(
    r"(class TypePrePass.*?void visit\(ImplDeclNode& node\) override \{)",
    r"\1\n            if (!node.genericParams.empty()) return;",
    content,
    flags=re.DOTALL
)

# Skip in ConstraintGenerator
content = re.sub(
    r"(class ConstraintGenerator.*?void visit\(FunctionDeclNode& node\) override \{)",
    r"\1\n              if (!node.genericParams.empty()) return;",
    content,
    flags=re.DOTALL
)
content = re.sub(
    r"(class ConstraintGenerator.*?void visit\(ImplDeclNode& node\) override \{)",
    r"\1\n              if (!node.genericParams.empty()) return;",
    content,
    flags=re.DOTALL
)

# Skip in TypeResolver (we must be careful not to skip registration!)
# In TypeResolver, we added:
#             if (!node.genericParams.empty() && monoEngine) {
#                 // register generic Impl blocks...
#             }
# If we return early, we skip the registration! So we should return AFTER registration!
# Let's fix TypeResolver manually.

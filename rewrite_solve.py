
import re

with open("src/MiddleEnd/TypeChecker.cpp", "r") as f:
    content = f.read()

start_idx = content.find("void solve(std::vector<Constraint> constraints) {")
end_idx = content.find("std::cerr << \"[DEBUG] Iterations maxed out!\\\\n\";", start_idx)

# Wait, we need to append new constraints safely.
# Inside the loop: `for (size_t i = 0; i < constraints.size(); ++i) { auto& c = constraints[i]; ... }`
# If we use an index-based loop `for (size_t i = 0; i < constraints.size(); ++i)`, it's safe to push_back!
# Because we access `c` by index `constraints[i]`. If it reallocates, `constraints[i]` still works as long as we re-fetch `c = constraints[i]` after modifying it, OR if we pass by value!
# But wait, `c` is passed by reference `for (const auto& c : constraints)` which uses iterators!

old_loop = "for (const auto& c : constraints) {"
new_loop = """for (size_t i = 0; i < constraints.size(); ++i) {
                  const auto c = constraints[i]; // Copy by value to prevent dangling references!"""
                  
content = content.replace(old_loop, new_loop)

# Let's also change PatternConstraintVisitor in EnumVariantPattern logic to NOT push Equality constraint, but instead do unify directly for c.expected.
# Wait, I already have rewrite_typechecker_match_3.py! 

with open("src/MiddleEnd/TypeChecker.cpp", "w") as f:
    f.write(content)

print("Rewritten solve loop to use index and copy-by-value!")

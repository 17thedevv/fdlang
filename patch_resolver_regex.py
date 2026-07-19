import re

with open('src/MiddleEnd/TypeChecker.cpp', 'r') as f:
    content = f.read()

# For TypeResolver::visit(ImplDeclNode)
impl_pat = r"(class TypeResolver.*?void visit\(ImplDeclNode& node\) override \{.*?monoEngine->registerGenericImpl\(targetStructId, &node\);\s*\}\s*\})"
impl_repl = r"\1\n            if (!node.genericParams.empty()) return;"

content = re.sub(impl_pat, impl_repl, content, flags=re.DOTALL)

# For TypeResolver::visit(FunctionDeclNode)
fn_pat = r"(class TypeResolver.*?void visit\(FunctionDeclNode& node\) override \{)(\s*if \(node\.body\))"
fn_repl = r"\1\n            if (!node.genericParams.empty()) return;\2"

content = re.sub(fn_pat, fn_repl, content, flags=re.DOTALL)

with open('src/MiddleEnd/TypeChecker.cpp', 'w') as f:
    f.write(content)
print("Patched TypeResolver with Regex")

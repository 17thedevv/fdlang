import re

with open('src/FrontEnd/Parser.cpp', 'r') as f:
    content = f.read()

pat = r"(auto structInit = std::make_unique<StructInitExpr>\(\);\s*structInit->loc = SourceLocation::fromOffset\(current\.byteOffset\);\s*structInit->path = idExpr->segments;)"
repl = r"\1\n                structInit->genericArgs = std::move(idExpr->genericArgs);"

content = re.sub(pat, repl, content)

with open('src/FrontEnd/Parser.cpp', 'w') as f:
    f.write(content)
print("Patched Parser.cpp for StructInitExpr genericArgs")

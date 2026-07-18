import re
with open('src/Support/Diagnostic.cpp', 'r') as f:
    content = f.read()
content = re.sub(
    r'std::cerr << "fdlang: error: " << message << "\\n";',
    r'std::cerr << "fdlang: error at byte " << loc.byteOffset << ": " << message << "\\n";',
    content
)
with open('src/Support/Diagnostic.cpp', 'w') as f:
    f.write(content)

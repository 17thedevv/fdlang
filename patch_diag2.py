import re
with open('src/Support/Diagnostic.cpp', 'r') as f:
    content = f.read()

content = re.sub(
    r'std::cerr << "fdlang: " << label << ": " << d\.message << \'\\n\';',
    r'std::cerr << "fdlang: byte: " << loc.byteOffset << " " << label << ": " << d.message << \'\\n\';',
    content
)

with open('src/Support/Diagnostic.cpp', 'w') as f:
    f.write(content)

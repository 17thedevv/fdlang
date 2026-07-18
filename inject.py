import sys
import re

with open('src/MiddleEnd/Resolver.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# Add #include <iostream> at the top safely
if '#include <iostream>' not in content:
    content = '#include <iostream>\n' + content

lines = content.splitlines()
out_lines = []

for l in lines:
    out_lines.append(l)
    if 'void visit(' in l and 'override' in l and '{' in l:
        m = re.search(r'void visit\(([A-Za-z0-9_]+)', l)
        if m:
            node = m.group(1)
            indent = l[:len(l) - len(l.lstrip())]
            out_lines.append(indent + f'    std::cout << "[Resolver] visit {node}\\n";')

with open('src/MiddleEnd/Resolver.cpp', 'w', encoding='utf-8') as f:
    f.write('\n'.join(out_lines))

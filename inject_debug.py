import sys
import re
content = open('src/MiddleEnd/Resolver.cpp', 'r', encoding='utf-8').read()

new_lines = []
for l in content.splitlines():
    new_lines.append(l)
    if l.strip().startswith('void visit(') and 'override' in l and '{' in l:
        match = re.search(r'void visit\(([A-Za-z0-9_]+)', l)
        if match:
            node_type = match.group(1)
            indent = l[:len(l) - len(l.lstrip())]
            new_lines.append(indent + f'    std::cout << "[Resolver] visit {node_type}\\n";')

with open('src/MiddleEnd/Resolver.cpp', 'w', encoding='utf-8') as f:
    f.write('\n'.join(new_lines))

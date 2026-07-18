import sys

with open('src/MiddleEnd/Resolver.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

lines = content.splitlines()
out = []
if '#include <iostream>' not in content:
    out.append('#include <iostream>')

for l in lines:
    if l.strip().startswith('void visit(') and 'override' in l and '{' in l:
        # replace the first '{' with '{ std::cout << "[Resolver] " << __FUNCTION__ << " line " << __LINE__ << std::endl;'
        idx = l.find('{')
        new_l = l[:idx+1] + ' std::cout << "[Resolver] " << __FUNCTION__ << " line " << __LINE__ << "\\n"; ' + l[idx+1:]
        out.append(new_l)
    else:
        out.append(l)

with open('src/MiddleEnd/Resolver.cpp', 'w', encoding='utf-8') as f:
    f.write('\n'.join(out))

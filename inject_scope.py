import sys
with open('src/MiddleEnd/Resolver.cpp', 'r', encoding='utf-8') as f:
    lines = f.read().splitlines()

out = []
for l in lines:
    if 'void enterScope(' in l and '{' in l:
        l = l.replace('{', '{ std::cout << "[Scope] enterScope\\n";')
    elif 'void enterExistingScope(' in l and '{' in l:
        l = l.replace('{', '{ std::cout << "[Scope] enterExistingScope\\n";')
    elif 'void exitScope(' in l and '{' in l:
        l = l.replace('{', '{ std::cout << "[Scope] exitScope\\n";')
    out.append(l)

with open('src/MiddleEnd/Resolver.cpp', 'w', encoding='utf-8') as f:
    f.write('\n'.join(out))

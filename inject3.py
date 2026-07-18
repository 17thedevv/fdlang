import sys
with open('src/MiddleEnd/Resolver.cpp', 'r', encoding='utf-8') as f:
    lines = f.read().splitlines()

out = []
for l in lines:
    if 'std::cout << "[Resolver] "' in l:
        l = l.replace('std::cout << "[Resolver] " << __FUNCTION__ << " line " << __LINE__ << "\\n";', 'printf("[Resolver] %s line %d\\n", __FUNCTION__, __LINE__); fflush(stdout);')
    if 'std::cout << "[Scope]' in l:
        l = l.replace('std::cout << "[Scope] enterScope\\n";', 'printf("[Scope] enterScope\\n"); fflush(stdout);')
        l = l.replace('std::cout << "[Scope] enterExistingScope\\n";', 'printf("[Scope] enterExistingScope\\n"); fflush(stdout);')
        l = l.replace('std::cout << "[Scope] exitScope\\n";', 'printf("[Scope] exitScope\\n"); fflush(stdout);')
    out.append(l)

with open('src/MiddleEnd/Resolver.cpp', 'w', encoding='utf-8') as f:
    f.write('\n'.join(out))

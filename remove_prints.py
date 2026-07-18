import sys
import re

with open('src/MiddleEnd/Resolver.cpp', 'r', encoding='utf-8') as f:
    lines = f.read().splitlines()

out = []
for l in lines:
    l = re.sub(r'printf\("\[Resolver\].*?\\n", __FUNCTION__, __LINE__\); fflush\(stdout\); *', '', l)
    l = re.sub(r'printf\("\[Scope\].*?\\n"\); fflush\(stdout\); *', '', l)
    # also remove any leftover std::cout from FLIRGen? No, FLIRGen used std::cout
    out.append(l)

with open('src/MiddleEnd/Resolver.cpp', 'w', encoding='utf-8') as f:
    f.write('\n'.join(out))

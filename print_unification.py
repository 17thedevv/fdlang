import sys
with open('src/MiddleEnd/TypeChecker.cpp', 'r') as f:
    lines = f.readlines()

for i, line in enumerate(lines):
    if "class UnificationEngine" in line:
        print("".join(lines[i:i+300]))
        break

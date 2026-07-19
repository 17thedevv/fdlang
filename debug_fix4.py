import re

with open('src/MiddleEnd/TypeChecker.cpp', 'r') as f:
    lines = f.readlines()

new_lines = []
i = 0
while i < len(lines):
    line = lines[i]
    if 'printf("[DEBUG]' in line and not line.rstrip().endswith(';'):
        # It's split across lines
        j = i + 1
        while j < len(lines) and '");' not in lines[j-1]:
            line = line.rstrip() + "\\n" + lines[j].lstrip()
            j += 1
        new_lines.append(line)
        i = j
    else:
        new_lines.append(line)
        i += 1

with open('src/MiddleEnd/TypeChecker.cpp', 'w') as f:
    f.writelines(new_lines)
print("Done")

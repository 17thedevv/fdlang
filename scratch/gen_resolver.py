import re

def extract_funcs(cpp_code):
    funcs = []
    
    # We find all matches for `void MacroValidator::visit(...`
    matches = list(re.finditer(r'void MacroValidator::visit\([A-Za-z]+&\s+node\)\s*\{', cpp_code))
    
    for match in matches:
        start_idx = match.start()
        
        # We need to find the matching closing brace
        brace_count = 0
        in_string = False
        escape = False
        end_idx = -1
        
        for i in range(match.end() - 1, len(cpp_code)):
            c = cpp_code[i]
            
            if in_string:
                if escape:
                    escape = False
                elif c == '\\':
                    escape = True
                elif c == '"':
                    in_string = False
            else:
                if c == '"':
                    in_string = True
                elif c == '{':
                    brace_count += 1
                elif c == '}':
                    brace_count -= 1
                    if brace_count == 0:
                        end_idx = i + 1
                        break
        
        if end_idx != -1:
            funcs.append(cpp_code[start_idx:end_idx])
            
    return funcs

with open('src/FrontEnd/MacroValidator.cpp', 'r') as f:
    content = f.read()
    
funcs = extract_funcs(content)

new_funcs = []
for f in funcs:
    if 'MacroCall' in f or 'ProgramNode' in f or 'MacroExpandForStmt' in f or 'MacroDeclNode' in f:
        continue
    f = f.replace('MacroValidator', 'MacroResolver')
    f = f.replace('this->typeVisitor()', '*this')
    f = f.replace('static_cast<PatternVisitor&>(*this)', '*this')
    new_funcs.append(f)

with open('src/FrontEnd/MacroResolver.cpp', 'a') as f:
    f.write('\n\n// Generated traversals\n')
    f.write('\n\n'.join(new_funcs))
    f.write('\n')

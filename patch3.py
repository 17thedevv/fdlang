import sys
content = open('src/MiddleEnd/TypeChecker.cpp', 'r', encoding='utf-8').read()

# 1. Add str to *const i8 coercion
coercion = """            if (auto* prim1 = dynamic_cast<const PrimitiveType*>(t1)) {
                if (prim1->builtinKind == BuiltinKind::Str) {
                    if (auto* ptr2 = dynamic_cast<const PointerType*>(t2)) {
                        if (auto* prim2 = dynamic_cast<const PrimitiveType*>(ptr2->pointee)) {
                            if (prim2->builtinKind == BuiltinKind::I8) return true;
                        }
                    }
                }
            }
            if (auto* ptr1 = dynamic_cast<const PointerType*>(t1)) {
                if (auto* prim2 = dynamic_cast<const PrimitiveType*>(t2)) {
                    if (prim2->builtinKind == BuiltinKind::Str) {
                        if (auto* prim1 = dynamic_cast<const PrimitiveType*>(ptr1->pointee)) {
                            if (prim1->builtinKind == BuiltinKind::I8) return true;
                        }
                    }
                }
                if (auto* ptr2 = dynamic_cast<const PointerType*>(t2)) {
                    if (ptr1->isMutable != ptr2->isMutable) goto mismatch;
                    return unify(ptr1->pointee, ptr2->pointee, loc);
                }
            }
"""
content = content.replace('''            if (auto* ptr1 = dynamic_cast<const PointerType*>(t1)) {
                if (auto* ptr2 = dynamic_cast<const PointerType*>(t2)) {
                    if (ptr1->isMutable != ptr2->isMutable) goto mismatch;
                    return unify(ptr1->pointee, ptr2->pointee, loc);
                }
            }''', coercion)


# 2. Fix FunctionType Variadic logic
func_logic_old = """                        if (hasNames) {
                            bool seenNamedArg = false;
                            std::vector<bool> provided(defSite->paramTypes.size(), false);
                            for (size_t i = 0; i < callSite->paramNames.size(); ++i) {
                                if (callSite->paramNames[i].empty()) { // Positional
                                    if (seenNamedArg) {
                                        diag.error(loc, "Positional argument cannot follow named arguments");
                                        return false;
                                    }
                                    if (provided[i]) {
                                        diag.error(loc, "Parameter provided multiple times");
                                        return false;
                                    }
                                    provided[i] = true;
                                    if (!unify(callSite->paramTypes[i], defSite->paramTypes[i], loc)) return false;
                                } else { // Named
                                    seenNamedArg = true;
                                    bool found = false;
                                    for (size_t j = 0; j < defSite->paramNames.size(); ++j) {
                                        if (defSite->paramNames[j] == callSite->paramNames[i]) {
                                            if (provided[j]) {
                                                diag.error(loc, "Parameter '" + callSite->paramNames[i] + "' provided multiple times");
                                                return false;
                                            }
                                            provided[j] = true;
                                            if (!unify(callSite->paramTypes[i], defSite->paramTypes[j], loc)) {
                                                return false;
                                            }
                                            found = true;
                                            break;
                                        }
                                    }
                                    if (!found) {
                                        diag.error(loc, "Named argument '" + callSite->paramNames[i] + "' not found in function signature");
                                        return false;
                                    }
                                }
                            }

                        } else {
                            for (size_t i = 0; i < fn1->paramTypes.size(); ++i) {
                                if (!unify(fn1->paramTypes[i], fn2->paramTypes[i], loc)) return false;
                            }
                        }
                    } else {
                        for (size_t i = 0; i < fn1->paramTypes.size(); ++i) {
                            if (!unify(fn1->paramTypes[i], fn2->paramTypes[i], loc)) return false;
                        }
                    }"""

func_logic_new = """                        if (defSite->isVariadic) {
                            if (callSite->paramTypes.size() < defSite->paramTypes.size()) goto mismatch;
                        } else {
                            if (callSite->paramTypes.size() != defSite->paramTypes.size()) goto mismatch;
                        }

                        if (hasNames) {
                            bool seenNamedArg = false;
                            std::vector<bool> provided(defSite->paramTypes.size(), false);
                            for (size_t i = 0; i < callSite->paramNames.size(); ++i) {
                                if (callSite->paramNames[i].empty()) { // Positional
                                    if (seenNamedArg) {
                                        diag.error(loc, "Positional argument cannot follow named arguments");
                                        return false;
                                    }
                                    if (i < provided.size() && provided[i]) {
                                        diag.error(loc, "Parameter provided multiple times");
                                        return false;
                                    }
                                    if (i < provided.size()) provided[i] = true;
                                    
                                    const Type* defType = (defSite->isVariadic && i >= defSite->paramTypes.size()) ? callSite->paramTypes[i] : defSite->paramTypes[i];
                                    if (!unify(callSite->paramTypes[i], defType, loc)) return false;
                                } else { // Named
                                    seenNamedArg = true;
                                    bool found = false;
                                    for (size_t j = 0; j < defSite->paramNames.size(); ++j) {
                                        if (defSite->paramNames[j] == callSite->paramNames[i]) {
                                            if (provided[j]) {
                                                diag.error(loc, "Parameter '" + callSite->paramNames[i] + "' provided multiple times");
                                                return false;
                                            }
                                            provided[j] = true;
                                            if (!unify(callSite->paramTypes[i], defSite->paramTypes[j], loc)) {
                                                return false;
                                            }
                                            found = true;
                                            break;
                                        }
                                    }
                                    if (!found) {
                                        diag.error(loc, "Named argument '" + callSite->paramNames[i] + "' not found in function signature");
                                        return false;
                                    }
                                }
                            }
                        } else {
                            for (size_t i = 0; i < callSite->paramTypes.size(); ++i) {
                                const Type* defType = (defSite->isVariadic && i >= defSite->paramTypes.size()) ? callSite->paramTypes[i] : defSite->paramTypes[i];
                                if (!unify(callSite->paramTypes[i], defType, loc)) return false;
                            }
                        }
                    } else {
                        if (fn1->isVariadic != fn2->isVariadic || fn1->paramTypes.size() != fn2->paramTypes.size()) goto mismatch;
                        for (size_t i = 0; i < fn1->paramTypes.size(); ++i) {
                            if (!unify(fn1->paramTypes[i], fn2->paramTypes[i], loc)) return false;
                        }
                    }"""

content = content.replace(func_logic_old, func_logic_new)
content = content.replace('                    if (fn1->paramTypes.size() != fn2->paramTypes.size()) goto mismatch;\n', '')

with open('src/MiddleEnd/TypeChecker.cpp', 'w', encoding='utf-8') as f:
    f.write(content)

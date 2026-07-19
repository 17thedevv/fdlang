
import re

with open("src/MiddleEnd/TypeChecker.cpp", "r") as f:
    content = f.read()

st_logic = """if (auto* st1 = dynamic_cast<const StructType*>(t1)) {
                  if (auto* st2 = dynamic_cast<const StructType*>(t2)) {
                      if (st1->structSymbolId != st2->structSymbolId) return false;
                      if (st1->genericArgs.size() != st2->genericArgs.size()) return false;
                      for (size_t i = 0; i < st1->genericArgs.size(); ++i) {
                          if (!unify(st1->genericArgs[i], st2->genericArgs[i], loc)) return false;
                      }
                      return true;
                  }
              }"""

enum_logic = """} else if (auto* e1 = dynamic_cast<const EnumType*>(t1)) {
                  if (auto* e2 = dynamic_cast<const EnumType*>(t2)) {
                      if (e1->enumSymbolId != e2->enumSymbolId) return false;
                      if (e1->genericArgs.size() != e2->genericArgs.size()) return false;
                      for (size_t i = 0; i < e1->genericArgs.size(); ++i) {
                          if (!unify(e1->genericArgs[i], e2->genericArgs[i], loc)) return false;
                      }
                      return true;
                  }
              }"""

if enum_logic not in content:
    content = content.replace(st_logic, st_logic + "\n              " + enum_logic)

with open("src/MiddleEnd/TypeChecker.cpp", "w") as f:
    f.write(content)
print("Added EnumType unify logic!")

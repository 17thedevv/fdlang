
import re

with open("src/MiddleEnd/TypeChecker.cpp", "r") as f:
    content = f.read()

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

new_enum_logic = """} else if (auto* e1 = dynamic_cast<const EnumType*>(t1)) {
                  if (auto* e2 = dynamic_cast<const EnumType*>(t2)) {
                      if (e1->enumSymbolId != e2->enumSymbolId) return false;
                      if (e1->genericArgs.size() != e2->genericArgs.size()) return false;
                      for (size_t i = 0; i < e1->genericArgs.size(); ++i) {
                          std::cerr << "[DEBUG] unifying enum args: " << e1->genericArgs[i]->toString() << " and " << e2->genericArgs[i]->toString() << "\\n";
                          if (!unify(e1->genericArgs[i], e2->genericArgs[i], loc)) {
                              std::cerr << "[DEBUG] Enum args unify failed!\\n";
                              return false;
                          }
                      }
                      return true;
                  }
              }"""

content = content.replace(enum_logic, new_enum_logic)

with open("src/MiddleEnd/TypeChecker.cpp", "w") as f:
    f.write(content)
print("Added enum debug!")

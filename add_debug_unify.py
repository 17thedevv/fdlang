
import re

with open("src/MiddleEnd/TypeChecker.cpp", "r") as f:
    content = f.read()

inf2_code = """if (inf2) {
                  ctx.unificationTable.unify(inf2->varId, t1);
                  return true;
              }"""
              
if "DEBUG unified inf2" not in content:
    content = content.replace(inf2_code, """if (inf2) {
                  std::cerr << "[DEBUG] unified inf2 " << inf2->varId << " with " << t1->toString() << "\\n";
                  ctx.unificationTable.unify(inf2->varId, t1);
                  return true;
              }""")

with open("src/MiddleEnd/TypeChecker.cpp", "w") as f:
    f.write(content)

print("Added debug to unify")

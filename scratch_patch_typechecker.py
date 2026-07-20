
import re

with open("src/MiddleEnd/TypeChecker.cpp", "r", encoding="utf-8") as f:
    content = f.read()

replacement = """        bool unify(const Type* t1, const Type* t2, SourceLocation loc) {
            t1 = ctx.unificationTable.deepResolve(t1, ctx);
            t2 = ctx.unificationTable.deepResolve(t2, ctx);
            if (!t1 || !t2) return false;
            if (t1->getKind() == TypeKind::Error || t2->getKind() == TypeKind::Error) return true;
            if (t1 == t2 || t1->equals(t2)) return true;"""

content = content.replace("""        bool unify(const Type* t1, const Type* t2, SourceLocation loc) {
            t1 = ctx.unificationTable.deepResolve(t1, ctx);
            t2 = ctx.unificationTable.deepResolve(t2, ctx);
            if (!t1 || !t2) return false;
            if (t1 == t2 || t1->equals(t2)) return true;""", replacement)

with open("src/MiddleEnd/TypeChecker.cpp", "w", encoding="utf-8") as f:
    f.write(content)

print("Patched TypeChecker unify")


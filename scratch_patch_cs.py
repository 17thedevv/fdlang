
import re

with open("src/Core/CompilerSession.cpp", "r", encoding="utf-8") as f:
    content = f.read()

content = content.replace("""    if (!tcOk) {
        diag_.error(SourceLocation::invalid(), "TypeChecker that bai, nhung tiep tuc de test MVIR...");
        diag_.flush();
        // return false;
    }""", """    if (!tcOk) {
        diag_.error(SourceLocation::invalid(), "Compilation aborted due to TypeChecker errors.");
        diag_.flush();
        return false;
    }""")

with open("src/Core/CompilerSession.cpp", "w", encoding="utf-8") as f:
    f.write(content)

print("Patched CompilerSession.cpp")


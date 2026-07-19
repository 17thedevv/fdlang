
import re

with open("src/MiddleEnd/TypeChecker.cpp", "r") as f:
    content = f.read()

content = content.replace("param.name.view()", "param.name")

with open("src/MiddleEnd/TypeChecker.cpp", "w") as f:
    f.write(content)

print("Rewritten param.name.view() to param.name in TypeChecker!")

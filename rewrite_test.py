
with open("tests/module_test/test_enums.ms", "r") as f:
    content = f.read()
content = content.replace("Option@<int_32>", "Option<int_32>")
with open("tests/module_test/test_enums.ms", "w") as f:
    f.write(content)
print("Updated test_enums.ms!")

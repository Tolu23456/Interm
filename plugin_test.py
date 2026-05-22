import interm

print("Python Plugin: Hello from INTERM Python Bridge!")
interm.insert(0, "Python was here!\n")
print(f"Python Plugin: Buffer start: {interm.get_text(0, 10)}")

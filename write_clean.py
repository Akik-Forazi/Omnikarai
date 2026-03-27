with open('/tmp/codegen_clean.c','r',encoding='utf-8') as f:
    data = f.read()
with open(r'C:\Users\akikf\programing\omnikarai\omniwin\src\codegen.c','w',encoding='utf-8') as f:
    f.write(data)
print(f"Written. Lines: {data.count(chr(10))}")

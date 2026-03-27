import sys
sys.stdout.reconfigure(encoding='utf-8')

with open(r'C:\Users\akikf\programing\omnikarai\omniwin\src\codegen.c','r',encoding='utf-8') as f:
    content = f.read()

# Show scope_define
idx = content.find('static Symbol* scope_define')
print(content[idx:idx+300])
print("---")
# Show scope_new
idx2 = content.find('static SymbolTable* scope_new')
print(content[idx2:idx2+300])

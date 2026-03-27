import re

with open(r'C:\Users\akikf\programing\omnikarai\omniwin\src\codegen.c', 'r', encoding='utf-8') as f:
    content = f.read()

# Find pkg block: starts after cg_continue_statement line, ends before g_known_modules
pkg_end = content.find('static const char* g_known_modules[]')
# Find start by looking for the PACKAGE MODULE LOADER comment
pkg_header = '//  PACKAGE MODULE LOADER'
pkg_comment_pos = content.find(pkg_header)
# Go back to the === line before it
pkg_start = content.rfind('// ===', 0, pkg_comment_pos)
pkg_block = content[pkg_start:pkg_end]

# Insert just before MODULE METHOD DISPATCH / cg_module_call
insert_marker = '//  MODULE METHOD DISPATCH'
insert_pos = content.rfind('// ===', 0, content.find(insert_marker))

new_content = (
    content[:insert_pos] +
    pkg_block +
    content[insert_pos:pkg_start] +
    content[pkg_end:]
)

with open(r'C:\Users\akikf\programing\omnikarai\omniwin\src\codegen.c', 'w', encoding='utf-8') as f:
    f.write(new_content)

print(f'Done. Lines: {new_content.count(chr(10))}')
# Verify positions
for label, marker in [
    ('Pkg loader', '//  PACKAGE MODULE LOADER'),
    ('cg_module_call', 'static void cg_module_call('),
    ('g_known_modules', 'static const char* g_known_modules'),
]:
    pos = new_content.find(marker)
    line = new_content[:pos].count('\n') + 1 if pos != -1 else -1
    print(f'  {label}: line {line}')

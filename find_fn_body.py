import sys
sys.stdout.reconfigure(encoding='utf-8')

with open(r'C:\Users\akikf\programing\omnikarai\omniwin\src\codegen.c', 'r', encoding='utf-8') as f:
    src = f.read()

# Find cg_fn_body and replace it completely with the correct version
old_fn_start = 'static void cg_fn_body(CodeGen*cg,AST_Statement_FnDef*fn_def){\n'
old_fn_end = '    scope_free(fn_scope);\n    cg->scope=saved_scope;cg->stack_size=saved_stack;cg->returned=saved_ret;\n    cg->reg_var_depth=saved_reg_depth;cg->in_main_body=1;\n    cg->self_slot=saved_self_slot;\n}\n'

start_idx = src.find(old_fn_start)
end_idx = src.find(old_fn_end, start_idx) + len(old_fn_end)

print(f"Found fn_body: chars {start_idx} to {end_idx}")
print(f"Block size: {end_idx-start_idx} chars")

old_block = src[start_idx:end_idx]
print("First 100:", repr(old_block[:100]))
print("Last 100:", repr(old_block[-100:]))

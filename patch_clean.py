import base64, os

# Read the clean codegen from outputs
src_path = r'C:\Users\akikf\programing\omnikarai\omniwin\src\codegen.c'

# We'll patch the current file using Python string operations
with open(src_path, 'r', encoding='utf-8') as f:
    src = f.read()

print(f"Current lines: {src.count(chr(10))}")

# === PATCH 1: Remove HOT VARIABLE ANALYZER and broken cg_while_statement ===
hva_start = src.find('/* \u2500\u2500 HOT VARIABLE ANALYZER')
for_range_start = src.find('\nstatic void cg_for_range(')
if hva_start > 0 and for_range_start > hva_start:
    old_block = src[hva_start:for_range_start]
    new_while = '''static void cg_while_statement(CodeGen*cg,AST_Statement_While*stmt){
    size_t loop_top=cg->code.size;
    int saved_break=cg->break_patch_count,saved_cont=cg->continue_patch_count;
    cg_expr(cg,stmt->condition);emit_test_rax(&cg->code);size_t je=emit_je_fwd(&cg->code);
    for(int i=0;i<stmt->body->statement_count;i++)cg_stmt(cg,stmt->body->statements[i]);
    for(int i=saved_cont;i<cg->continue_patch_count;i++){int32_t d=(int32_t)(loop_top-(cg->continue_patches[i]+4));patch_u32(&cg->code,cg->continue_patches[i],(uint32_t)d);}
    cg->continue_patch_count=saved_cont;
    emit_jmp_back(&cg->code,loop_top);resolve_fwd(&cg->code,je);
    for(int i=saved_break;i<cg->break_patch_count;i++)resolve_fwd(&cg->code,cg->break_patches[i]);
    cg->break_patch_count=saved_break;
}

'''
    src = src[:hva_start] + new_while + src[for_range_start:]
    print("PATCH 1 applied: removed HOT ANALYZER, restored simple while")
else:
    print(f"PATCH 1: hva_start={hva_start}, for_range_start={for_range_start}")

# === PATCH 2: Remove broken callee-saves from cg_fn_body ===
bad_saves = '''    emit_u32(&cg->code,256);
    /* Save callee-saved regs that may be used for variable pinning (rbx,r12,r13)
       Windows x64 ABI requires these to be preserved across calls.
       We save them at a fixed offset inside the frame that we reserve here.
       We always save all 3 to keep the frame layout predictable. */
    /* MOV [rbp-8],  RBX */ emit_u8(&cg->code,REX_W); emit_u8(&cg->code,0x89); emit_u8(&cg->code,0x5D); emit_u8(&cg->code,0xF8);
    /* MOV [rbp-16], R12 */ emit_u8(&cg->code,0x4C);  emit_u8(&cg->code,0x89); emit_u8(&cg->code,0x65); emit_u8(&cg->code,0xF0);
    /* MOV [rbp-24], R13 */ emit_u8(&cg->code,0x4C);  emit_u8(&cg->code,0x89); emit_u8(&cg->code,0x6D); emit_u8(&cg->code,0xE8);'''
good_saves = '    emit_u32(&cg->code,256);'
if src.count(bad_saves) == 1:
    src = src.replace(bad_saves, good_saves, 1)
    print("PATCH 2 applied: removed broken callee-saves")
else:
    print(f"PATCH 2: bad_saves count = {src.count(bad_saves)}")

with open(src_path, 'w', encoding='utf-8') as f:
    f.write(src)
print(f"Written. Lines: {src.count(chr(10))}")

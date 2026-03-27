import sys
sys.stdout.reconfigure(encoding='utf-8')

with open(r'C:\Users\akikf\programing\omnikarai\omniwin\src\codegen.c','r',encoding='utf-8') as f:
    src = f.read()

print(f"Lines before: {src.count(chr(10))}")

# ── PATCH: Fix the register allocator frame layout ──────────────────────────
# 
# PROBLEM: Variables start at [rbp-8], but we want to save rbx/r12/r13 there.
# SOLUTION: When ra_pinned > 0:
#   - Reserve [rbp-8]=rbx, [rbp-16]=r12, [rbp-24]=r13 saves
#   - Set fn_scope->next_offset = 56 (skip the save zone + shadow space)
#   - Set stack_size = 56 minimum
#
# Also PROBLEM: cg_return_statement doesn't restore callee-saved regs.
# SOLUTION: Check reg_var_depth in return and emit restores.
#
# Also PROBLEM: The ra_count loop triples counts unconditionally for EVERY while/for
# which means outer loops inflate inner variable counts wrong.
# SOLUTION: Only multiply inner-most loop variables.

# ── Fix 1: fn_body scope initialization — start vars AFTER save zone ────────
OLD1 = '''    cg->scope=fn_scope;cg->stack_size=32;cg->returned=0;cg->reg_var_depth=0;
    memset(cg->reg_var_names,0,sizeof(cg->reg_var_names));
    memset(cg->reg_var_saved,0,sizeof(cg->reg_var_saved));
    cg->in_main_body=0;'''

NEW1 = '''    cg->scope=fn_scope;cg->stack_size=32;cg->returned=0;cg->reg_var_depth=0;
    memset(cg->reg_var_names,0,sizeof(cg->reg_var_names));
    memset(cg->reg_var_saved,0,sizeof(cg->reg_var_saved));
    cg->in_main_body=0;
    /* NOTE: ra_pinned is computed below and if >0, we'll bump fn_scope->next_offset
       after computing it. Done after the register analysis block. */'''

c1 = src.count(OLD1)
print(f"Fix 1 count: {c1}")
if c1 == 1:
    src = src.replace(OLD1, NEW1, 1)
    print("Fix 1 applied")

# ── Fix 2: After reg_var_depth assignment, set scope start offset ────────────
OLD2 = '''    cg->reg_var_depth = ra_pinned; /* slots 0,1 (r14/r15) unused in fn body */

    /* Prologue: save callee-saved regs we're about to use.
       Stack layout: [rbp-8]=rbx, [rbp-16]=r12, [rbp-24]=r13
       Local vars start at offset 56 (32 shadow + 24 saves). */'''

NEW2 = '''    cg->reg_var_depth = ra_pinned; /* slots 0,1 (r14/r15) unused in fn body */

    /* If we're pinning registers, reserve the top of the frame for saves.
       Frame layout: [rbp-8]=rbx, [rbp-16]=r12, [rbp-24]=r13 (callee saves)
       Variables then start at offset 56 (32 shadow + 24 save zone).
       Without pinning: variables start at 8 (the default). */
    if(ra_pinned > 0){
        fn_scope->next_offset = 56;
        cg->stack_size = 56;
    }

    /* Prologue: save callee-saved regs we're about to use.
       Stack layout: [rbp-8]=rbx, [rbp-16]=r12, [rbp-24]=r13
       Local vars start at offset 56 (32 shadow + 24 saves). */'''

c2 = src.count(OLD2)
print(f"Fix 2 count: {c2}")
if c2 == 1:
    src = src.replace(OLD2, NEW2, 1)
    print("Fix 2 applied")

# ── Fix 3: Also initialize pinned vars with their initial value ──────────────
# After save zone setup + prologue saves, load pinned vars from stack into regs
# But at this point pinned vars don't have stack slots yet (defined during body emit)
# So we DON'T preload — we just let the first SET emit to the register.
# This is already correct.

# ── Fix 4: Fix cg_return_statement to restore callee-saved regs ─────────────
OLD4 = '''static void cg_return_statement(CodeGen*cg,AST_Statement_Return*stmt){
    if(stmt->return_value)cg_expr(cg,stmt->return_value);else emit_xor_rax_rax(&cg->code);
    if(cg->in_main_body){
        emit_u8(&cg->code,0x4C);emit_u8(&cg->code,0x8B);emit_u8(&cg->code,0x75);emit_u8(&cg->code,0xF8); // mov r14,[rbp-8]
        emit_u8(&cg->code,0x4C);emit_u8(&cg->code,0x8B);emit_u8(&cg->code,0x7D);emit_u8(&cg->code,0xF0); // mov r15,[rbp-16]
    }'''

NEW4 = '''static void cg_return_statement(CodeGen*cg,AST_Statement_Return*stmt){
    if(stmt->return_value)cg_expr(cg,stmt->return_value);else emit_xor_rax_rax(&cg->code);
    if(cg->in_main_body){
        emit_u8(&cg->code,0x4C);emit_u8(&cg->code,0x8B);emit_u8(&cg->code,0x75);emit_u8(&cg->code,0xF8); // mov r14,[rbp-8]
        emit_u8(&cg->code,0x4C);emit_u8(&cg->code,0x8B);emit_u8(&cg->code,0x7D);emit_u8(&cg->code,0xF0); // mov r15,[rbp-16]
    } else {
        /* Inside a function: restore callee-saved regs if we pinned any */
        int rp = cg->reg_var_depth;
        if(rp>=3){ /* MOV R13,[rbp-24] */ emit_u8(&cg->code,0x4C);emit_u8(&cg->code,0x8B);emit_u8(&cg->code,0x6D);emit_u8(&cg->code,0xE8); }
        if(rp>=2){ /* MOV R12,[rbp-16] */ emit_u8(&cg->code,0x4C);emit_u8(&cg->code,0x8B);emit_u8(&cg->code,0x65);emit_u8(&cg->code,0xF0); }
        if(rp>=1){ /* MOV RBX,[rbp-8]  */ emit_u8(&cg->code,REX_W);emit_u8(&cg->code,0x8B);emit_u8(&cg->code,0x5D);emit_u8(&cg->code,0xF8); }
    }'''

c4 = src.count(OLD4)
print(f"Fix 4 count: {c4}")
if c4 == 1:
    src = src.replace(OLD4, NEW4, 1)
    print("Fix 4 applied")

# ── Fix 5: Remove the tripling in ra_count_stmt — it causes wrong counts ─────
# The tripling makes outer variables look hotter than inner ones incorrectly.
# Better: just count raw. The inner loop variables will naturally appear more
# because the loop body executes more statements.
OLD5 = '''        case WHILE_STATEMENT:{
            AST_Statement_While*ws=(AST_Statement_While*)s;
            ra_count_expr(ws->condition,va,nv);
            // Inner loop vars get weight multiplied by LOOP_WEIGHT (simulate hot path)
            int pre_nv=*nv;
            ra_count_block(ws->body,va,nv);
            // Triple the read/write counts for vars active in while body (hot path boost)
            for(int i=0;i<*nv;i++){va[i].reads*=3;va[i].writes*=3;}
            break;
        }
        case FOR_STATEMENT:{
            AST_Statement_For*fs=(AST_Statement_For*)s;
            ra_count_block(fs->body,va,nv);
            for(int i=0;i<*nv;i++){va[i].reads*=3;va[i].writes*=3;}
            break;
        }'''

NEW5 = '''        case WHILE_STATEMENT:{
            AST_Statement_While*ws=(AST_Statement_While*)s;
            ra_count_expr(ws->condition,va,nv);
            /* Count body vars multiple times to simulate hot path weight */
            for(int _w=0;_w<8;_w++) ra_count_block(ws->body,va,nv);
            break;
        }
        case FOR_STATEMENT:{
            AST_Statement_For*fs=(AST_Statement_For*)s;
            for(int _w=0;_w<8;_w++) ra_count_block(fs->body,va,nv);
            break;
        }'''

c5 = src.count(OLD5)
print(f"Fix 5 count: {c5}")
if c5 == 1:
    src = src.replace(OLD5, NEW5, 1)
    print("Fix 5 applied")

with open(r'C:\Users\akikf\programing\omnikarai\omniwin\src\codegen.c','w',encoding='utf-8') as f:
    f.write(src)
print(f"Lines after: {src.count(chr(10))}")

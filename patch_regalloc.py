import re

with open(r'C:\Users\akikf\programing\omnikarai\omniwin\src\codegen.c', 'r', encoding='utf-8') as f:
    src = f.read()

# ── CHANGE 1: Add set_done label at end of cg_set_statement ──────────────────
old1 = '                sym->type=OMNI_TYPE_UNKNOWN; // mark as instance type\n            }\n        }\n    }\n}\n\nstatic void cg_if_statement'
new1 = '                sym->type=OMNI_TYPE_UNKNOWN; // mark as instance type\n            }\n        }\n    }\n    set_done:;\n}\n\nstatic void cg_if_statement'
src = src.replace(old1, new1, 1)
print("Change 1 (set_done label):", old1[:60] in src or new1[:60] in src)

# ── CHANGE 2: Replace cg_while_statement with version that pins hot vars ─────
old2 = '''static void cg_while_statement(CodeGen*cg,AST_Statement_While*stmt){
    size_t loop_top=cg->code.size;
    int saved_break=cg->break_patch_count,saved_cont=cg->continue_patch_count;
    cg_expr(cg,stmt->condition);emit_test_rax(&cg->code);size_t je=emit_je_fwd(&cg->code);
    // No child scope: variables set inside a while body must persist across
    // iterations and remain visible after the loop ends (e.g. for return stmts).
    for(int i=0;i<stmt->body->statement_count;i++)cg_stmt(cg,stmt->body->statements[i]);
    for(int i=saved_cont;i<cg->continue_patch_count;i++){int32_t d=(int32_t)(loop_top-(cg->continue_patches[i]+4));patch_u32(&cg->code,cg->continue_patches[i],(uint32_t)d);}
    cg->continue_patch_count=saved_cont;
    emit_jmp_back(&cg->code,loop_top);resolve_fwd(&cg->code,je);
    for(int i=saved_break;i<cg->break_patch_count;i++)resolve_fwd(&cg->code,cg->break_patches[i]);
    cg->break_patch_count=saved_break;
}'''

new2 = '''/* ── HOT VARIABLE ANALYZER ─────────────────────────────────────────────────
   Count how many times each variable is read (GET) in a block.
   Used to pick the hottest variables to pin in registers before a while loop.
   Slot mapping: 2=rbx, 3=r12, 4=r13  (0,1 are r14/r15 reserved for for-loops) */
#define HOT_MAX 32
typedef struct { char name[64]; int count; } HotVar;
static void count_expr_reads(AST_Expression*e, HotVar*hv, int*nhv){
    if(!e) return;
    switch(e->type){
        case IDENTIFIER:{
            const char*n=((AST_Expression_Identifier*)e)->value;
            for(int i=0;i<*nhv;i++) if(!strcmp(hv[i].name,n)){hv[i].count++;return;}
            if(*nhv<HOT_MAX){strncpy_s(hv[*nhv].name,64,n,_TRUNCATE);hv[(*nhv)++].count=1;}
            break;
        }
        case INFIX_EXPRESSION:{AST_Expression_Infix*in=(AST_Expression_Infix*)e;count_expr_reads(in->left,hv,nhv);count_expr_reads(in->right,hv,nhv);break;}
        case PREFIX_EXPRESSION:{count_expr_reads(((AST_Expression_Prefix*)e)->right,hv,nhv);break;}
        case CALL_EXPRESSION:{
            AST_Expression_Call*c=(AST_Expression_Call*)e;
            count_expr_reads(c->function,hv,nhv);
            for(int i=0;i<c->argument_count;i++) count_expr_reads(c->arguments[i],hv,nhv);
            break;
        }
        default: break;
    }
}
static void count_block_reads(AST_Statement_Block*blk, HotVar*hv, int*nhv){
    if(!blk) return;
    for(int i=0;i<blk->statement_count;i++){
        AST_Statement*s=blk->statements[i]; if(!s) continue;
        switch(s->type){
            case SET_STATEMENT:   count_expr_reads(((AST_Statement_Set*)s)->value,hv,nhv); break;
            case RETURN_STATEMENT:count_expr_reads(((AST_Statement_Return*)s)->return_value,hv,nhv); break;
            case EXPRESSION_STATEMENT:count_expr_reads(((AST_Statement_Expression*)s)->expression,hv,nhv); break;
            case IF_STATEMENT:{
                AST_Statement_If*is=(AST_Statement_If*)s;
                count_expr_reads(is->condition,hv,nhv);
                count_block_reads(is->consequence,hv,nhv);
                if(is->alternative&&is->alternative->type==IF_STATEMENT)
                    count_block_reads(((AST_Statement_If*)is->alternative)->consequence,hv,nhv);
                break;
            }
            case WHILE_STATEMENT:{
                AST_Statement_While*ws=(AST_Statement_While*)s;
                count_expr_reads(ws->condition,hv,nhv);
                count_block_reads(ws->body,hv,nhv);
                break;
            }
            default: break;
        }
    }
}

/* Emit MOV reg←[stack] for a pinned register slot */
static void emit_load_pinned(CodeBuf*b, int slot, int stack_off){
    /* Load the variable's current stack value into its assigned register */
    emit_u8(b,0x48+((slot>=3)?1:0)); /* REX.W or REX.WB for r12/r13 */
    switch(slot){
        case 2: emit_u8(b,0x8B); emit_u8(b,0x9D); emit_u32(b,(uint32_t)(-stack_off)); break; /* MOV RBX,[rbp-off] */
        case 3: emit_u8(b,0x8B); /* MOV R12,[rbp-off] */ emit_u8(b,0xA5); emit_u32(b,(uint32_t)(-stack_off)); break;
        case 4: emit_u8(b,0x8B); /* MOV R13,[rbp-off] */ emit_u8(b,0xAD); emit_u32(b,(uint32_t)(-stack_off)); break;
        default: break;
    }
}
/* Emit MOV [stack]←reg for writeback */
static void emit_store_pinned(CodeBuf*b, int slot, int stack_off){
    emit_u8(b,0x48+((slot>=3)?1:0));
    switch(slot){
        case 2: emit_u8(b,0x89); emit_u8(b,0x9D); emit_u32(b,(uint32_t)(-stack_off)); break; /* MOV [rbp-off],RBX */
        case 3: emit_u8(b,0x89); emit_u8(b,0xA5); emit_u32(b,(uint32_t)(-stack_off)); break; /* MOV [rbp-off],R12 */
        case 4: emit_u8(b,0x89); emit_u8(b,0xAD); emit_u32(b,(uint32_t)(-stack_off)); break; /* MOV [rbp-off],R13 */
        default: break;
    }
}

static void cg_while_statement(CodeGen*cg,AST_Statement_While*stmt){
    /* OPT: Before emitting body, find the 3 hottest variables and pin them
       in rbx/r12/r13 (slots 2-4). This eliminates stack loads/stores for
       inner loop variables — the key to matching C speed on compute loops. */
    int nhv=0; HotVar hv[HOT_MAX];
    count_expr_reads(stmt->condition, hv, &nhv);
    count_block_reads(stmt->body, hv, &nhv);

    /* Sort by read count descending (simple insertion sort — small array) */
    for(int i=1;i<nhv;i++){HotVar tmp=hv[i];int j=i-1;while(j>=0&&hv[j].count<tmp.count){hv[j+1]=hv[j];j--;}hv[j+1]=tmp;}

    /* Pin up to 3 hot variables in slots 2,3,4 — if slot is free and
       variable already exists in scope (is defined before the loop). */
    int n_pinned=0;
    int pinned_slots[3]={-1,-1,-1};
    int pinned_stack[3]={0,0,0};
    char pinned_names[3][64]={};

    for(int hi=0;hi<nhv&&n_pinned<3;hi++){
        /* Skip if already pinned in slots 0/1 (r14/r15 for for-loops) */
        int already=0;
        for(int ri=0;ri<cg->reg_var_depth&&ri<5;ri++)
            if(!strcmp(cg->reg_var_names[ri],hv[hi].name)){already=1;break;}
        if(already) continue;

        /* Must be defined in scope already (not first-use inside loop) */
        Symbol*sym=scope_get(cg->scope,hv[hi].name);
        if(!sym) continue;

        /* Must have decent use count to justify register — at least 3 reads */
        if(hv[hi].count<3) break;

        int slot=2+n_pinned; /* slots 2,3,4 = rbx,r12,r13 */
        if(cg->reg_var_depth>=5) break; /* all slots taken */

        /* Pin it */
        strncpy_s(cg->reg_var_names[cg->reg_var_depth],64,hv[hi].name,_TRUNCATE);
        cg->reg_var_depth++;
        pinned_slots[n_pinned]=slot;
        pinned_stack[n_pinned]=sym->stack_offset;
        strncpy_s(pinned_names[n_pinned],64,hv[hi].name,_TRUNCATE);
        n_pinned++;

        /* Save register on stack first (preserve caller's value) */
        /* We use push/pop: push before loop, pop after */
        switch(slot){
            case 2: emit_push_rbx(&cg->code); break;
            case 3: emit_push_r12(&cg->code); break;
            case 4: emit_push_r13(&cg->code); break;
        }
        /* Load current stack value into register */
        emit_load_pinned(&cg->code, slot, sym->stack_offset);
    }

    size_t loop_top=cg->code.size;
    int saved_break=cg->break_patch_count,saved_cont=cg->continue_patch_count;
    cg_expr(cg,stmt->condition);emit_test_rax(&cg->code);size_t je=emit_je_fwd(&cg->code);
    for(int i=0;i<stmt->body->statement_count;i++)cg_stmt(cg,stmt->body->statements[i]);
    for(int i=saved_cont;i<cg->continue_patch_count;i++){int32_t d=(int32_t)(loop_top-(cg->continue_patches[i]+4));patch_u32(&cg->code,cg->continue_patches[i],(uint32_t)d);}
    cg->continue_patch_count=saved_cont;
    emit_jmp_back(&cg->code,loop_top);resolve_fwd(&cg->code,je);
    for(int i=saved_break;i<cg->break_patch_count;i++)resolve_fwd(&cg->code,cg->break_patches[i]);
    cg->break_patch_count=saved_break;

    /* Writeback pinned registers to stack and restore callee-saved regs */
    for(int pi=n_pinned-1;pi>=0;pi--){
        int slot=pinned_slots[pi];
        /* Write final value back to stack slot */
        emit_store_pinned(&cg->code, slot, pinned_stack[pi]);
        /* Pop restored value (LIFO order) */
        switch(slot){
            case 2: emit_pop_rbx(&cg->code); break;
            case 3: emit_pop_r12(&cg->code); break;
            case 4: emit_pop_r13(&cg->code); break;
        }
        /* Unpin: remove from reg_var_names */
        cg->reg_var_depth--;
    }
}'''

count = src.count(old2)
print(f"Change 2 match count: {count}")
if count == 1:
    src = src.replace(old2, new2, 1)
    print("Change 2 applied OK")
else:
    print("FAIL - old2 not found exactly once")
    # Try to find it
    idx = src.find('static void cg_while_statement')
    if idx >= 0:
        print(f"Found cg_while_statement at char {idx}")
        print(repr(src[idx:idx+200]))

with open(r'C:\Users\akikf\programing\omnikarai\omniwin\src\codegen.c', 'w', encoding='utf-8') as f:
    f.write(src)
print(f"Written. Total lines: {src.count(chr(10))}")

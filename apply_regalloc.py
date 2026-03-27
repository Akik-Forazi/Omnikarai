with open(r'C:\Users\akikf\programing\omnikarai\omniwin\src\codegen.c', 'r', encoding='utf-8') as f:
    src = f.read()

# ============================================================
# PATCH: Add function-level register allocator into cg_fn_body
# Variables assigned to rbx(slot2)/r12(slot3)/r13(slot4)
# Saved at [rbp-8/16/24], locals start at stack_size=64
# ============================================================

# --- STEP 1: Add hot-var counter before cg_fn_body ---
# Find the right insertion point: just before "static void cg_fn_body"
FN_BODY_MARKER = 'static void cg_fn_body(CodeGen*cg,AST_Statement_FnDef*fn_def){'

HOT_VAR_CODE = '''
// ============================================================
//  FUNCTION-LEVEL REGISTER ALLOCATOR
//  Counts variable use frequency in a function body and pins
//  the 3 hottest locals into rbx/r12/r13 (callee-saved).
//  This eliminates stack load/store for hot inner-loop vars.
// ============================================================

#define RA_MAX 64
typedef struct { char name[64]; int reads; int writes; } RAVarInfo;

static void ra_count_expr(AST_Expression*e, RAVarInfo*va, int*nv){
    if(!e)return;
    switch(e->type){
        case IDENTIFIER:{
            const char*n=((AST_Expression_Identifier*)e)->value;
            // Skip built-in names
            if(!strcmp(n,"math")||!strcmp(n,"self"))return;
            for(int i=0;i<*nv;i++) if(!strcmp(va[i].name,n)){va[i].reads++;return;}
            if(*nv<RA_MAX){strncpy_s(va[*nv].name,64,n,_TRUNCATE);va[*nv].reads=1;va[*nv].writes=0;(*nv)++;}
            break;
        }
        case INFIX_EXPRESSION:{AST_Expression_Infix*in=(AST_Expression_Infix*)e;ra_count_expr(in->left,va,nv);ra_count_expr(in->right,va,nv);break;}
        case PREFIX_EXPRESSION:{ra_count_expr(((AST_Expression_Prefix*)e)->right,va,nv);break;}
        case CALL_EXPRESSION:{
            AST_Expression_Call*c=(AST_Expression_Call*)e;
            for(int i=0;i<c->argument_count;i++) ra_count_expr(c->arguments[i],va,nv);
            break;
        }
        case INDEX_EXPRESSION:{AST_Expression_Index*ix=(AST_Expression_Index*)e;ra_count_expr(ix->left,va,nv);ra_count_expr(ix->index,va,nv);break;}
        default:break;
    }
}
static void ra_count_block(AST_Statement_Block*blk, RAVarInfo*va, int*nv);
static void ra_count_stmt(AST_Statement*s, RAVarInfo*va, int*nv){
    if(!s)return;
    switch(s->type){
        case SET_STATEMENT:{
            AST_Statement_Set*ss=(AST_Statement_Set*)s;
            // Count the destination as a write
            const char*dn=ss->name?ss->name->value:NULL;
            if(dn){
                int found=0;
                for(int i=0;i<*nv;i++) if(!strcmp(va[i].name,dn)){va[i].writes++;found=1;break;}
                if(!found&&*nv<RA_MAX){strncpy_s(va[*nv].name,64,dn,_TRUNCATE);va[*nv].reads=0;va[*nv].writes=1;(*nv)++;}
            }
            ra_count_expr(ss->value,va,nv);
            break;
        }
        case RETURN_STATEMENT:ra_count_expr(((AST_Statement_Return*)s)->return_value,va,nv);break;
        case EXPRESSION_STATEMENT:ra_count_expr(((AST_Statement_Expression*)s)->expression,va,nv);break;
        case IF_STATEMENT:{
            AST_Statement_If*is=(AST_Statement_If*)s;
            ra_count_expr(is->condition,va,nv);
            ra_count_block(is->consequence,va,nv);
            if(is->alternative) ra_count_stmt(is->alternative,va,nv);
            break;
        }
        case WHILE_STATEMENT:{
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
        }
        default:break;
    }
}
static void ra_count_block(AST_Statement_Block*blk, RAVarInfo*va, int*nv){
    if(!blk)return;
    for(int i=0;i<blk->statement_count;i++) ra_count_stmt(blk->statements[i],va,nv);
}

// Sort RAVarInfo by (reads+writes) DESC — simple insertion sort
static void ra_sort(RAVarInfo*va, int nv){
    for(int i=1;i<nv;i++){
        RAVarInfo tmp=va[i]; int j=i-1;
        while(j>=0 && (va[j].reads+va[j].writes)<(tmp.reads+tmp.writes)){va[j+1]=va[j];j--;}
        va[j+1]=tmp;
    }
}

'''

count = src.count(FN_BODY_MARKER)
print(f"FN_BODY_MARKER count: {count}")
if count == 1:
    src = src.replace(FN_BODY_MARKER, HOT_VAR_CODE + FN_BODY_MARKER, 1)
    print("STEP 1 applied")

# --- STEP 2: Update cg_fn_body to use register allocation ---
# Find the fn_body after params are stored, before body emission
OLD_FN_BODY = '''    for(int i=0;i<fn_def->body->statement_count;i++)cg_stmt(cg,fn_def->body->statements[i]);

    if(!cg->returned){
        emit_xor_rax_rax(&cg->code);
        emit_mov_rsp_rbp(&cg->code);
        emit_pop_rbp(&cg->code);
        emit_ret(&cg->code);
    }'''

NEW_FN_BODY = '''    /* ── FUNCTION-LEVEL REGISTER ALLOCATION ──────────────────────────────────
       Analyze the function body, find the 3 hottest variables, pin them to
       rbx(slot2)/r12(slot3)/r13(slot4). Save/restore in prologue/epilogue.
       Frame layout: [rbp-8]=saved_rbx, [rbp-16]=saved_r12, [rbp-24]=saved_r13
       First local variable starts at stack_offset = 56 (after shadow+saves).
       This eliminates all stack traffic for hot inner-loop variables. */
    RAVarInfo ra_vars[RA_MAX]; int ra_nv=0;
    ra_count_block(fn_def->body, ra_vars, &ra_nv);
    ra_sort(ra_vars, ra_nv);

    /* Pick top vars to pin — skip parameters (already stored at known offsets).
       Only pin vars that are both read AND written (true loop variables). */
    int ra_pinned=0;
    static const char* RA_REG_NAMES[3]={"rbx","r12","r13"};
    for(int ri=0;ri<ra_nv && ra_pinned<3; ri++){
        RAVarInfo*rv=&ra_vars[ri];
        /* Must be both read and written — pure read-only (like params passed in) skip */
        if(rv->reads<2||rv->writes<1) continue;
        /* Skip params — they already have proper stack slots */
        int is_param=0;
        for(int pi=0;pi<fn_def->parameter_count;pi++)
            if(!strcmp(fn_def->parameters[pi]->value,rv->name)){is_param=1;break;}
        if(is_param) continue;
        /* Assign to slot 2+ra_pinned (rbx/r12/r13) */
        int slot=2+ra_pinned;
        strncpy_s(cg->reg_var_names[slot],64,rv->name,_TRUNCATE);
        ra_pinned++;
    }
    cg->reg_var_depth = ra_pinned; /* slots 0,1 (r14/r15) unused in fn body */

    /* Prologue: save callee-saved regs we're about to use.
       Stack layout: [rbp-8]=rbx, [rbp-16]=r12, [rbp-24]=r13
       Local vars start at offset 56 (32 shadow + 24 saves). */
    if(ra_pinned>=1){
        /* MOV [rbp-8], RBX */
        emit_u8(&cg->code,REX_W);emit_u8(&cg->code,0x89);emit_u8(&cg->code,0x5D);emit_u8(&cg->code,0xF8);
    }
    if(ra_pinned>=2){
        /* MOV [rbp-16], R12 */
        emit_u8(&cg->code,0x4C);emit_u8(&cg->code,0x89);emit_u8(&cg->code,0x65);emit_u8(&cg->code,0xF0);
    }
    if(ra_pinned>=3){
        /* MOV [rbp-24], R13 */
        emit_u8(&cg->code,0x4C);emit_u8(&cg->code,0x89);emit_u8(&cg->code,0x6D);emit_u8(&cg->code,0xE8);
    }
    /* Reserve space for saves: first local var at offset 56 */
    if(ra_pinned>0 && cg->stack_size<56) cg->stack_size=56;

    for(int i=0;i<fn_def->body->statement_count;i++)cg_stmt(cg,fn_def->body->statements[i]);

    /* For each pinned variable, write register back to its stack slot so
       caller code or any function exit path sees the final value */
    /* (stack sync is already done by cg_set_statement for pinned vars) */

    if(!cg->returned){
        emit_xor_rax_rax(&cg->code);
        /* Epilogue: restore callee-saved regs */
        if(ra_pinned>=3){
            /* MOV R13, [rbp-24] */
            emit_u8(&cg->code,0x4C);emit_u8(&cg->code,0x8B);emit_u8(&cg->code,0x6D);emit_u8(&cg->code,0xE8);
        }
        if(ra_pinned>=2){
            /* MOV R12, [rbp-16] */
            emit_u8(&cg->code,0x4C);emit_u8(&cg->code,0x8B);emit_u8(&cg->code,0x65);emit_u8(&cg->code,0xF0);
        }
        if(ra_pinned>=1){
            /* MOV RBX, [rbp-8] */
            emit_u8(&cg->code,REX_W);emit_u8(&cg->code,0x8B);emit_u8(&cg->code,0x5D);emit_u8(&cg->code,0xF8);
        }
        emit_mov_rsp_rbp(&cg->code);
        emit_pop_rbp(&cg->code);
        emit_ret(&cg->code);
    }'''

count2 = src.count(OLD_FN_BODY)
print(f"OLD_FN_BODY count: {count2}")
if count2 == 1:
    src = src.replace(OLD_FN_BODY, NEW_FN_BODY, 1)
    print("STEP 2 applied")

# --- STEP 3: Handle the returned=1 path in cg_return_statement ---
# Find cg_return_statement to add reg restores before every ret
OLD_RETURN = '''static void cg_return_statement(CodeGen*cg,AST_Statement_Return*stmt){'''

# We need to add epilogue before every return
# Find the cg_return_statement function
ret_idx = src.find('static void cg_return_statement(CodeGen*cg,AST_Statement_Return*stmt){')
print(f"cg_return_statement at char {ret_idx}")
if ret_idx > 0:
    # Find the end of this function (next static void)
    next_fn = src.find('\nstatic void ', ret_idx+1)
    ret_fn = src[ret_idx:next_fn]
    print(f"Return fn size: {len(ret_fn)} chars")
    # Show it
    print(ret_fn[:500])

with open(r'C:\Users\akikf\programing\omnikarai\omniwin\src\codegen.c', 'w', encoding='utf-8') as f:
    f.write(src)
print(f"Written. Lines: {src.count(chr(10))}")

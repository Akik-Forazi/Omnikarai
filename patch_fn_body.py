import sys
sys.stdout.reconfigure(encoding='utf-8')

with open(r'C:\Users\akikf\programing\omnikarai\omniwin\src\codegen.c', 'r', encoding='utf-8') as f:
    src = f.read()

old_fn_start = 'static void cg_fn_body(CodeGen*cg,AST_Statement_FnDef*fn_def){\n'
old_fn_end = '    scope_free(fn_scope);\n    cg->scope=saved_scope;cg->stack_size=saved_stack;cg->returned=saved_ret;\n    cg->reg_var_depth=saved_reg_depth;cg->in_main_body=1;\n    cg->self_slot=saved_self_slot;\n}\n'

start_idx = src.find(old_fn_start)
end_idx = src.find(old_fn_end, start_idx) + len(old_fn_end)

NEW_FN = r'''static void cg_fn_body(CodeGen*cg,AST_Statement_FnDef*fn_def){
    FnEntry*fe=fn_find(cg,fn_def->name->value);
    if(!fe){fprintf(stderr,"CodeGen Error: fn_table entry missing for '%s'\n",fn_def->name->value);exit(1);}
    fe->code_offset=cg->code.size;fe->resolved=1;
    if(fe->is_inline)return;
    BETA_TRACE_CG("fn_body '%s' params=%d offset=%zu",fn_def->name->value,fn_def->parameter_count,fe->code_offset);

    /* REGISTER ALLOCATOR: analyze body BEFORE emitting prologue.
       We save callee regs (rbx/r12/r13) into high frame slots [rbp-232/240/248]
       so variables keep their normal layout starting at [rbp-8] for first param.
       This avoids ANY conflict between callee saves and variable slots. */
    SymbolTable*fn_scope=scope_new(NULL);
    SymbolTable*saved_scope=cg->scope;
    int saved_stack=cg->stack_size,saved_ret=cg->returned,saved_reg_depth=cg->reg_var_depth;
    int saved_self_slot=cg->self_slot;

    cg->scope=fn_scope;cg->stack_size=32;cg->returned=0;cg->reg_var_depth=0;
    memset(cg->reg_var_names,0,sizeof(cg->reg_var_names));
    memset(cg->reg_var_saved,0,sizeof(cg->reg_var_saved));
    cg->in_main_body=0;

    /* Analyze body: count reads per variable, sort hottest first */
    HotVar ra_hv[HOT_MAX]; int ra_nhv=0;
    count_block_reads(fn_def->body, ra_hv, &ra_nhv);
    for(int i=1;i<ra_nhv;i++){HotVar tmp=ra_hv[i];int j=i-1;while(j>=0&&ra_hv[j].count<tmp.count){ra_hv[j+1]=ra_hv[j];j--;}ra_hv[j+1]=tmp;}

    /* Pick top 3 non-param hottest variables for rbx/r12/r13 (slots 2/3/4) */
    int ra_pinned=0;
    for(int ri=0;ri<ra_nhv&&ra_pinned<3;ri++){
        if(ra_hv[ri].count<4) break;
        int is_param=0;
        for(int pi=0;pi<fn_def->parameter_count;pi++)
            if(!strcmp(fn_def->parameters[pi]->value,ra_hv[ri].name)){is_param=1;break;}
        if(is_param) continue;
        strncpy_s(cg->reg_var_names[2+ra_pinned],64,ra_hv[ri].name,_TRUNCATE);
        ra_pinned++;
    }
    /* reg_var_depth = 2+ra_pinned: slots 0,1="" (r14/r15 unused in fns), 2-4=pinned vars */
    cg->reg_var_depth = 2 + ra_pinned;
    if(g_beta){fprintf(stderr,"[REGALLOC] fn='%s' pinned=%d",fn_def->name->value,ra_pinned);for(int _d=0;_d<ra_pinned;_d++)fprintf(stderr," '%s'",cg->reg_var_names[2+_d]);fprintf(stderr,"\n");}

    /* Emit prologue: push rbp; mov rbp,rsp; sub rsp,N */
    emit_push_rbp(&cg->code);
    emit_mov_rbp_rsp(&cg->code);
    emit_u8(&cg->code,REX_W);emit_u8(&cg->code,0x81);emit_u8(&cg->code,0xEC);
    size_t stack_patch=cg->code.size;
    emit_u32(&cg->code,256);

    /* Save callee-saved regs into HIGH frame slots (232/240/248 from rbp).
       These are near the TOP of the 256-byte frame, well above any variables.
       Variables start at [rbp-8] (first param) as usual -- NO layout change. */
    if(ra_pinned>=1){emit_u8(&cg->code,0x48);emit_u8(&cg->code,0x89);emit_u8(&cg->code,0x9D);emit_u32(&cg->code,(uint32_t)(-232));} /* MOV [rbp-232],RBX */
    if(ra_pinned>=2){emit_u8(&cg->code,0x4C);emit_u8(&cg->code,0x89);emit_u8(&cg->code,0xA5);emit_u32(&cg->code,(uint32_t)(-240));} /* MOV [rbp-240],R12 */
    if(ra_pinned>=3){emit_u8(&cg->code,0x4C);emit_u8(&cg->code,0x89);emit_u8(&cg->code,0xAD);emit_u32(&cg->code,(uint32_t)(-248));} /* MOV [rbp-248],R13 */

    /* Store parameters from ABI regs into frame at normal offsets */
    for(int i=0;i<fn_def->parameter_count;i++){
        const char*pname=fn_def->parameters[i]->value;
        Symbol*p_sym=scope_define(fn_scope,pname,OMNI_TYPE_INT);
        if(i==0&&(!strcmp(pname,"self")))cg->self_slot=p_sym->stack_offset;
        if(p_sym->stack_offset>cg->stack_size)cg->stack_size=p_sym->stack_offset;
        switch(i){
            case 0:emit_u8(&cg->code,REX_W);emit_u8(&cg->code,0x89);emit_u8(&cg->code,0x8D);emit_u32(&cg->code,(uint32_t)(-p_sym->stack_offset));break;
            case 1:emit_u8(&cg->code,REX_W);emit_u8(&cg->code,0x89);emit_u8(&cg->code,0x95);emit_u32(&cg->code,(uint32_t)(-p_sym->stack_offset));break;
            case 2:emit_u8(&cg->code,REX_WR);emit_u8(&cg->code,0x89);emit_u8(&cg->code,0x85);emit_u32(&cg->code,(uint32_t)(-p_sym->stack_offset));break;
            case 3:emit_u8(&cg->code,REX_WR);emit_u8(&cg->code,0x89);emit_u8(&cg->code,0x8D);emit_u32(&cg->code,(uint32_t)(-p_sym->stack_offset));break;
        }
    }

    /* Emit function body */
    for(int i=0;i<fn_def->body->statement_count;i++)cg_stmt(cg,fn_def->body->statements[i]);

    /* Epilogue: restore callee-saved regs, then return */
    if(!cg->returned){
        emit_xor_rax_rax(&cg->code);
        if(ra_pinned>=3){emit_u8(&cg->code,0x4C);emit_u8(&cg->code,0x8B);emit_u8(&cg->code,0xAD);emit_u32(&cg->code,(uint32_t)(-248));} /* MOV R13,[rbp-248] */
        if(ra_pinned>=2){emit_u8(&cg->code,0x4C);emit_u8(&cg->code,0x8B);emit_u8(&cg->code,0xA5);emit_u32(&cg->code,(uint32_t)(-240));} /* MOV R12,[rbp-240] */
        if(ra_pinned>=1){emit_u8(&cg->code,0x48);emit_u8(&cg->code,0x8B);emit_u8(&cg->code,0x9D);emit_u32(&cg->code,(uint32_t)(-232));} /* MOV RBX,[rbp-232] */
        emit_mov_rsp_rbp(&cg->code);
        emit_pop_rbp(&cg->code);
        emit_ret(&cg->code);
    }

    patch_u32(&cg->code,stack_patch,aligned_frame(cg->stack_size));

    scope_free(fn_scope);
    cg->scope=saved_scope;cg->stack_size=saved_stack;cg->returned=saved_ret;
    cg->reg_var_depth=saved_reg_depth;cg->in_main_body=1;
    cg->self_slot=saved_self_slot;
}
'''

new_src = src[:start_idx] + NEW_FN + src[end_idx:]
print(f"Lines: {new_src.count(chr(10))}")

with open(r'C:\Users\akikf\programing\omnikarai\omniwin\src\codegen.c', 'w', encoding='utf-8') as f:
    f.write(new_src)
print("Written OK")

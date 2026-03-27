import sys
sys.stdout.reconfigure(encoding='utf-8')

with open(r'C:\Users\akikf\programing\omnikarai\omniwin\src\codegen.c', 'r', encoding='utf-8') as f:
    src = f.read()

# Also fix cg_return_statement epilogue to use high offsets
old_ret = '''    } else {
        /* Restore callee-saved regs pinned by register allocator (LIFO) */
        int rp=cg->reg_var_depth;
        /* rp = 2+n_pinned: rp>=3 means 1 pinned(rbx), rp>=4 means 2(+r12), rp>=5 means 3(+r13) */
        /* MOV R13,[rbp-24] */ if(rp>=5){emit_u8(&cg->code,0x4C);emit_u8(&cg->code,0x8B);emit_u8(&cg->code,0x6D);emit_u8(&cg->code,0xE8);}
        /* MOV R12,[rbp-16] */ if(rp>=4){emit_u8(&cg->code,0x4C);emit_u8(&cg->code,0x8B);emit_u8(&cg->code,0x65);emit_u8(&cg->code,0xF0);}
        /* MOV RBX,[rbp-8]  */ if(rp>=3){emit_u8(&cg->code,0x48);emit_u8(&cg->code,0x8B);emit_u8(&cg->code,0x5D);emit_u8(&cg->code,0xF8);}
    }'''

new_ret = '''    } else {
        /* Restore callee-saved regs pinned by register allocator.
           rp = 2+n_pinned: rp>=3=rbx pinned, rp>=4=+r12, rp>=5=+r13
           Saved at high frame offsets 232/240/248 to avoid variable conflicts. */
        int rp=cg->reg_var_depth;
        if(rp>=5){emit_u8(&cg->code,0x4C);emit_u8(&cg->code,0x8B);emit_u8(&cg->code,0xAD);emit_u32(&cg->code,(uint32_t)(-248));} /* MOV R13,[rbp-248] */
        if(rp>=4){emit_u8(&cg->code,0x4C);emit_u8(&cg->code,0x8B);emit_u8(&cg->code,0xA5);emit_u32(&cg->code,(uint32_t)(-240));} /* MOV R12,[rbp-240] */
        if(rp>=3){emit_u8(&cg->code,0x48);emit_u8(&cg->code,0x8B);emit_u8(&cg->code,0x9D);emit_u32(&cg->code,(uint32_t)(-232));} /* MOV RBX,[rbp-232] */
    }'''

count = src.count(old_ret)
print(f"cg_return_statement epilogue matches: {count}")
if count == 1:
    src = src.replace(old_ret, new_ret, 1)
    print("Fixed")

with open(r'C:\Users\akikf\programing\omnikarai\omniwin\src\codegen.c', 'w', encoding='utf-8') as f:
    f.write(src)
print(f"Done. Lines: {src.count(chr(10))}")

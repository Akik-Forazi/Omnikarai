with open(r'C:\Users\akikf\programing\omnikarai\omniwin\src\codegen.c', 'r', encoding='utf-8') as f:
    src = f.read()

# Remove the dead if(0){ block and old duplicate constructor tagging
old = '''    // placeholder \u2014 constructor tagging now inside block above
    if(0){
    // If RHS is a constructor call (ClassName(...)), tag this variable as an instance
    // so p.method() can resolve to ClassName_method(p)
    if(stmt->value&&stmt->value->type==CALL_EXPRESSION){
        AST_Expression_Call*c=(AST_Expression_Call*)stmt->value;
        if(c->function&&c->function->type==IDENTIFIER){
            const char*cname=((AST_Expression_Identifier*)c->function)->value;
            if(class_find(cg,cname)){
                // Store a tag: "__class_ClassName" with stack_offset matching the instance var
                char tag[128];snprintf(tag,sizeof(tag),"__class_%s",cname);
                Symbol*tag_sym=scope_get(cg->scope,tag);
                if(!tag_sym){tag_sym=scope_define(cg->scope,tag,OMNI_TYPE_UNKNOWN);}
                tag_sym->stack_offset=sym->stack_offset;
                // Also store per-variable tag for field reads
                char vtag[128];snprintf(vtag,sizeof(vtag),"__class_%s_slot",stmt->name->value);
                Symbol*vtag_sym=scope_get(cg->scope,vtag);
                if(!vtag_sym){vtag_sym=scope_define(cg->scope,vtag,OMNI_TYPE_UNKNOWN);}
                // Find class index
                for(int ci=0;ci<cg->class_count;ci++){
                    if(!strcmp(cg->class_table[ci].class_name,cname)){
                        vtag_sym->stack_offset=ci;break;
                    }
                }
                sym->type=OMNI_TYPE_UNKNOWN; // mark as instance type
            }
        }
    }
    set_done:;
}'''

new = '''    set_done:;
}'''

count = src.count(old)
print(f"Match count: {count}")
if count == 1:
    src = src.replace(old, new, 1)
    print("Replaced OK")
else:
    print("NOT FOUND - trying fallback")
    # Find exact text around set_done
    idx = src.find('set_done:;')
    if idx >= 0:
        print(f"set_done at char {idx}")
        print(repr(src[idx-200:idx+50]))

with open(r'C:\Users\akikf\programing\omnikarai\omniwin\src\codegen.c', 'w', encoding='utf-8') as f:
    f.write(src)
print(f"Lines: {src.count(chr(10))}")

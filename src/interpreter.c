#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "interpreter.h"
#include "ast.h"
#include "object.h"

// --- Forward declarations for static functions ---
static Object* eval(AST_Node* node, Environment* env);
static int is_truthy(Object* obj);

// --- Helper Functions ---
static Object* new_integer_object(long long value) {
    Object* obj = malloc(sizeof(Object));
    obj->type = OBJ_INTEGER;
    obj->value.integer = value;
    return obj;
}

static Object* new_boolean_object(int value) {
    Object* obj = malloc(sizeof(Object));
    obj->type = OBJ_BOOLEAN;
    obj->value.boolean = value;
    return obj;
}

static Object* new_nil_object() {
    Object* obj = malloc(sizeof(Object));
    obj->type = OBJ_NIL;
    return obj;
}

static Object* new_string_object(char* value) {
    Object* obj = malloc(sizeof(Object));
    obj->type = OBJ_STRING;
    obj->value.string = strdup(value); // Duplicate the string
    return obj;
}

static Object* new_return_value_object(Object* value) {
    Object* obj = malloc(sizeof(Object));
    obj->type = OBJ_RETURN_VALUE;
    obj->value.return_value = value;
    return obj;
}

static Object* new_function_object(AST_Expression_Identifier** params, int param_count, AST_Statement_Block* body, Environment* env) {
    Object* obj = malloc(sizeof(Object));
    obj->type = OBJ_FUNCTION;
    ObjectFunction* fn = malloc(sizeof(ObjectFunction));
    fn->parameters = params;
    fn->parameter_count = param_count;
    fn->body = body;
    fn->env = env;
    obj->value.function = fn;
    return obj;
}

// --- Environment ---
typedef struct EnvEntry {
    char* name;
    Object* value;
    struct EnvEntry* next;
} EnvEntry;

struct Environment {
    EnvEntry* store;
    struct Environment* outer;
};

Environment* new_environment() {
    Environment* env = malloc(sizeof(Environment));
    env->store = NULL;
    env->outer = NULL;
    return env;
}

Object* get_environment(Environment* env, char* name) {
    EnvEntry* entry = env->store;
    while (entry != NULL) {
        if (strcmp(entry->name, name) == 0) {
            return entry->value;
        }
        entry = entry->next;
    }
    if (env->outer != NULL) {
        return get_environment(env->outer, name);
    }
    return NULL; // Not found
}

void set_environment(Environment* env, char* name, Object* val) {
    EnvEntry* entry = env->store;
    while (entry != NULL) {
        if (strcmp(entry->name, name) == 0) {
            entry->value = val;
            return;
        }
        entry = entry->next;
    }

    // Not found, add new entry
    EnvEntry* new_entry = malloc(sizeof(EnvEntry));
    new_entry->name = strdup(name);
    new_entry->value = val;
    new_entry->next = env->store;
    env->store = new_entry;
}

// --- Function Application ---
static Environment* extend_function_env(ObjectFunction* fn, Object** args, int arg_count) {
    Environment* env = new_environment();
    env->outer = fn->env; // Set the function's closure as the outer environment

    for (int i = 0; i < arg_count; i++) {
        set_environment(env, fn->parameters[i]->value, args[i]);
    }
    return env;
}

static Object* apply_function(Object* func, Object** args, int arg_count) {
    if (func->type != OBJ_FUNCTION) {
        fprintf(stderr, "RuntimeError: Expected a function, but got type %d.\n", func->type);
        exit(1);
    }

    ObjectFunction* fn = func->value.function;

    if (arg_count != fn->parameter_count) {
        fprintf(stderr, "RuntimeError: Wrong number of arguments. Expected %d, got %d.\n", fn->parameter_count, arg_count);
        exit(1);
    }

    Environment* extended_env = extend_function_env(fn, args, arg_count);
    Object* evaluated = eval((AST_Node*)fn->body, extended_env);

    // TODO: Free extended_env

    if (evaluated != NULL && evaluated->type == OBJ_RETURN_VALUE) {
        return evaluated->value.return_value; // Unwrap the return value
    }

    return evaluated;
}

// --- Interpreter ---

static Object* eval_program(AST_Program* program, Environment* env) {
    Object* result = NULL;
    for (int i = 0; i < program->statement_count; i++) {
        result = eval((AST_Node*)program->statements[i], env);
    }
    return result;
}

static Object* eval_set_statement(AST_Statement_Set* stmt, Environment* env) {
    Object* val = eval((AST_Node*)stmt->value, env);
    if (val == NULL) { // Error handling for evaluation
        return NULL; // Or an error object
    }
    set_environment(env, stmt->name->value, val);
    return val;
}

static Object* eval_identifier(AST_Expression_Identifier* ident, Environment* env) {
    Object* val = get_environment(env, ident->value);
    if (val == NULL) {
        // TODO: Create a proper error object
        fprintf(stderr, "RuntimeError: Identifier '%s' not found.\n", ident->value);
        exit(1);
    }
    return val;
}

static Object* eval_infix_expression(AST_Expression_Infix* infix, Environment* env) {
    Object* left = eval((AST_Node*)infix->left, env);
    Object* right = eval((AST_Node*)infix->right, env);

    // Only handle integer arithmetic for now
    if (left->type == OBJ_INTEGER && right->type == OBJ_INTEGER) {
        long long left_val = left->value.integer;
        long long right_val = right->value.integer;

        if (strcmp(infix->operator, "+") == 0) {
            return new_integer_object(left_val + right_val);
        } else if (strcmp(infix->operator, "-") == 0) {
            return new_integer_object(left_val - right_val);
        } else if (strcmp(infix->operator, "*") == 0) {
            return new_integer_object(left_val * right_val);
        } else if (strcmp(infix->operator, "/") == 0) {
            if (right_val == 0) {
                fprintf(stderr, "RuntimeError: Division by zero.\n");
                exit(1);
            }
            return new_integer_object(left_val / right_val);
        } else if (strcmp(infix->operator, "==") == 0) {
            return new_boolean_object(left_val == right_val);
        } else if (strcmp(infix->operator, "!=") == 0) {
            return new_boolean_object(left_val != right_val);
        } else if (strcmp(infix->operator, "<") == 0) {
            return new_boolean_object(left_val < right_val);
        } else if (strcmp(infix->operator, ">") == 0) {
            return new_boolean_object(left_val > right_val);
        } else if (strcmp(infix->operator, "<=") == 0) {
            return new_boolean_object(left_val <= right_val);
        } else if (strcmp(infix->operator, ">=") == 0) {
            return new_boolean_object(left_val >= right_val);
        }
    } else if (left->type == OBJ_STRING && right->type == OBJ_STRING) {
        if (strcmp(infix->operator, "+") == 0) {
            char* left_val = left->value.string;
            char* right_val = right->value.string;
            int new_len = strlen(left_val) + strlen(right_val);
            char* new_str = malloc(new_len + 1);
            strcpy(new_str, left_val);
            strcat(new_str, right_val);
            return new_string_object(new_str);
        }
    }
    // TODO: Handle other types and errors
    return NULL;
}

static Object* eval_prefix_expression(char* operator, Object* right) {
    if (strcmp(operator, "!") == 0) {
        if (is_truthy(right)) {
            return new_boolean_object(0);
        } else {
            return new_boolean_object(1);
        }
    } else if (strcmp(operator, "-") == 0) {
        if (right->type != OBJ_INTEGER) {
            // TODO: Error handling
            return new_nil_object();
        }
        long long value = right->value.integer;
        return new_integer_object(-value);
    }
    // TODO: Error handling for unknown operator
    return new_nil_object();
}

static int is_truthy(Object* obj) {
    if (obj == NULL || obj->type == OBJ_NIL) {
        return 0; // nil and NULL are falsy
    }
    if (obj->type == OBJ_BOOLEAN) {
        return obj->value.boolean;
    }
    // All other objects (integers, strings, etc.) are truthy for now
    return 1;
}

static Object* eval_block_statement(AST_Statement_Block* block, Environment* env) {
    Object* result = NULL;
    for (int i = 0; i < block->statement_count; i++) {
        result = eval((AST_Node*)block->statements[i], env);
        if (result != NULL && result->type == OBJ_RETURN_VALUE) {
            return result; // Propagate return value up
        }
    }
    return result;
}

static Object* eval_if_statement(AST_Statement_If* if_stmt, Environment* env) {
    Object* condition = eval((AST_Node*)if_stmt->condition, env);

    if (is_truthy(condition)) {
        return eval_block_statement(if_stmt->consequence, env);
    } else if (if_stmt->alternative != NULL) {
        // Recursively evaluate elif/else
        return eval((AST_Node*)if_stmt->alternative, env);
    } else {
        return new_nil_object(); // No alternative, return nil
    }
}

static Object** eval_expressions(AST_Expression** exprs, int count, Environment* env) {
    Object** result = malloc(count * sizeof(Object*));
    if (result == NULL) {
        fprintf(stderr, "Fatal: Memory allocation failed for evaluated expressions.\n");
        exit(1);
    }
    for (int i = 0; i < count; i++) {
        result[i] = eval((AST_Node*)exprs[i], env);
    }
    return result;
}

static Object* eval(AST_Node* node, Environment* env) {
    switch (node->type) {
        case EXPRESSION_STATEMENT:
            return eval((AST_Node*)((AST_Statement_Expression*)node)->expression, env);
        case INTEGER_LITERAL:
            return new_integer_object(((AST_Expression_IntegerLiteral*)node)->value);
        case BOOLEAN_LITERAL:
            return new_boolean_object(((AST_Expression_Boolean*)node)->value);
        case NIL_LITERAL:
            return new_nil_object();
        case STRING_LITERAL:
            return new_string_object(((AST_Expression_StringLiteral*)node)->value);
        case SET_STATEMENT:
            return eval_set_statement((AST_Statement_Set*)node, env);
        case IDENTIFIER:
            return eval_identifier((AST_Expression_Identifier*)node, env);
        case INFIX_EXPRESSION:
            return eval_infix_expression((AST_Expression_Infix*)node, env);
        case PREFIX_EXPRESSION: {
            AST_Expression_Prefix* prefix_expr = (AST_Expression_Prefix*)node;
            Object* right = eval((AST_Node*)prefix_expr->right, env);
            return eval_prefix_expression(prefix_expr->operator, right);
        }
        case IF_STATEMENT:
            return eval_if_statement((AST_Statement_If*)node, env);
        case BLOCK_STATEMENT: // This case is needed for consequence and alternative blocks
            return eval_block_statement((AST_Statement_Block*)node, env);
        case RETURN_STATEMENT: {
            AST_Statement_Return* ret_stmt = (AST_Statement_Return*)node;
            Object* val = eval((AST_Node*)ret_stmt->return_value, env);
            return new_return_value_object(val);
        }
        case FN_DEFINITION: {
            AST_Statement_FnDef* fn_def = (AST_Statement_FnDef*)node;
            Object* fn_obj = new_function_object(fn_def->parameters, fn_def->parameter_count, fn_def->body, env);
            set_environment(env, fn_def->name->value, fn_obj);
            return fn_obj;
        }
        case CALL_EXPRESSION: {
            AST_Expression_Call* call_expr = (AST_Expression_Call*)node;
            Object* function = eval((AST_Node*)call_expr->function, env);
            if (function == NULL) return NULL; // Error handling

            Object** args = eval_expressions(call_expr->arguments, call_expr->argument_count, env);
            if (args == NULL) return NULL; // Error handling

            Object* result = apply_function(function, args, call_expr->argument_count);
            // TODO: Free args array
            return result;
        }
        default:
            return NULL;
    }
}

Object* interpret(AST_Program* program) {
    Environment* env = new_environment();
    return eval_program(program, env);
}

void print_object(Object* obj) {
    if (obj == NULL) {
        printf("NULL\n");
        return;
    }
    switch (obj->type) {
        case OBJ_INTEGER:
            printf("%lld", obj->value.integer);
            break;
        case OBJ_BOOLEAN:
            printf("%s", obj->value.boolean ? "true" : "false");
            break;
        case OBJ_NIL:
            printf("nil");
            break;
        case OBJ_STRING:
            printf("%s", obj->value.string);
            break;
        case OBJ_RETURN_VALUE:
            print_object(obj->value.return_value);
            break;
        case OBJ_FUNCTION:
            printf("<function>");
            break;
        default:
            printf("Unknown object type\n");
            break;
    }
}

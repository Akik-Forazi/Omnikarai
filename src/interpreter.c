#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "interpreter.h"
#include "ast.h"
#include "object.h"

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

// --- Interpreter ---

static Object* eval(AST_Node* node, Environment* env);

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
    }
    // TODO: Handle other types and errors
    return NULL;
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
        case IF_STATEMENT:
            return eval_if_statement((AST_Statement_If*)node, env);
        case BLOCK_STATEMENT: // This case is needed for consequence and alternative blocks
            return eval_block_statement((AST_Statement_Block*)node, env);
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
        default:
            printf("Unknown object type\n");
            break;
    }
}

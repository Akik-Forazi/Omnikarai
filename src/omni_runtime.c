#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "omni_runtime.h"

// --- Object Creation Functions ---
OmniValue omni_new_integer(long long val) {
    OmniValue obj;
    obj.type = OMNI_INTEGER;
    obj.value.integer = val;
    return obj;
}

OmniValue omni_new_boolean(int val) {
    OmniValue obj;
    obj.type = OMNI_BOOLEAN;
    obj.value.boolean = val;
    return obj;
}

OmniValue omni_new_nil() {
    OmniValue obj;
    obj.type = OMNI_NIL;
    // No value for nil
    return obj;
}

OmniValue omni_new_string(const char* val) {
    OmniValue obj;
    obj.type = OMNI_STRING;
    obj.value.string = strdup(val); // Duplicate the string for ownership
    return obj;
}

// --- Runtime Functions ---
void omni_print(OmniValue val) {
    switch (val.type) {
        case OMNI_INTEGER:
            printf("%lld\n", val.value.integer);
            break;
        case OMNI_BOOLEAN:
            printf("%s\n", val.value.boolean ? "true" : "false");
            break;
        case OMNI_NIL:
            printf("nil\n");
            break;
        case OMNI_STRING:
            printf("%s\n", val.value.string);
            break;
        default:
            printf("Unknown OmniValue type for print.\n");
            break;
    }
}

// --- Binary Operations (Example: Integer Addition) ---
OmniValue omni_add(OmniValue left, OmniValue right) {
    if (left.type == OMNI_INTEGER && right.type == OMNI_INTEGER) {
        return omni_new_integer(left.value.integer + right.value.integer);
    } else if (left.type == OMNI_STRING && right.type == OMNI_STRING) {
        // String concatenation
        size_t len = strlen(left.value.string) + strlen(right.value.string) + 1;
        char* new_str = malloc(len);
        if (new_str == NULL) { /* Handle error */ exit(1); }
        strcpy(new_str, left.value.string);
        strcat(new_str, right.value.string);
        return omni_new_string(new_str);
    }
    // TODO: Handle type errors and other type combinations
    fprintf(stderr, "Runtime Error: Unsupported types for addition.\n");
    exit(1);
}

OmniValue omni_subtract(OmniValue left, OmniValue right) {
    if (left.type == OMNI_INTEGER && right.type == OMNI_INTEGER) {
        return omni_new_integer(left.value.integer - right.value.integer);
    }
    fprintf(stderr, "Runtime Error: Unsupported types for subtraction.\n");
    exit(1);
}

OmniValue omni_multiply(OmniValue left, OmniValue right) {
    if (left.type == OMNI_INTEGER && right.type == OMNI_INTEGER) {
        return omni_new_integer(left.value.integer * right.value.integer);
    }
    fprintf(stderr, "Runtime Error: Unsupported types for multiplication.\n");
    exit(1);
}

OmniValue omni_divide(OmniValue left, OmniValue right) {
    if (left.type == OMNI_INTEGER && right.type == OMNI_INTEGER) {
        if (right.value.integer == 0) {
            fprintf(stderr, "Runtime Error: Division by zero.\n");
            exit(1);
        }
        return omni_new_integer(left.value.integer / right.value.integer);
    }
    fprintf(stderr, "Runtime Error: Unsupported types for division.\n");
    exit(1);
}

// --- Comparison Operations ---
// Helper for general equality check
static int omni_values_equal(OmniValue left, OmniValue right) {
    if (left.type != right.type) return 0;

    switch (left.type) {
        case OMNI_INTEGER: return left.value.integer == right.value.integer;
        case OMNI_BOOLEAN: return left.value.boolean == right.value.boolean;
        case OMNI_NIL: return 1; // nil == nil
        case OMNI_STRING: return strcmp(left.value.string, right.value.string) == 0;
        default: return 0; // Unsupported types for direct comparison
    }
}

OmniValue omni_equal(OmniValue left, OmniValue right) {
    return omni_new_boolean(omni_values_equal(left, right));
}

OmniValue omni_not_equal(OmniValue left, OmniValue right) {
    return omni_new_boolean(!omni_values_equal(left, right));
}

OmniValue omni_less_than(OmniValue left, OmniValue right) {
    if (left.type == OMNI_INTEGER && right.type == OMNI_INTEGER) {
        return omni_new_boolean(left.value.integer < right.value.integer);
    }
    fprintf(stderr, "Runtime Error: Unsupported types for less than comparison.\n");
    exit(1);
}

OmniValue omni_greater_than(OmniValue left, OmniValue right) {
    if (left.type == OMNI_INTEGER && right.type == OMNI_INTEGER) {
        return omni_new_boolean(left.value.integer > right.value.integer);
    }
    fprintf(stderr, "Runtime Error: Unsupported types for greater than comparison.\n");
    exit(1);
}

OmniValue omni_less_than_equal(OmniValue left, OmniValue right) {
    if (left.type == OMNI_INTEGER && right.type == OMNI_INTEGER) {
        return omni_new_boolean(left.value.integer <= right.value.integer);
    }
    fprintf(stderr, "Runtime Error: Unsupported types for less than or equal comparison.\n");
    exit(1);
}

OmniValue omni_greater_than_equal(OmniValue left, OmniValue right) {
    if (left.type == OMNI_INTEGER && right.type == OMNI_INTEGER) {
        return omni_new_boolean(left.value.integer >= right.value.integer);
    }
    fprintf(stderr, "Runtime Error: Unsupported types for greater than or equal comparison.\n");
    exit(1);
}

// --- Truthiness ---
int omni_is_truthy(OmniValue val) {
    if (val.type == OMNI_NIL) {
        return 0; // nil is falsy
    }
    if (val.type == OMNI_BOOLEAN) {
        return val.value.boolean; // true/false are as is
    }
    // For now, all other types (integers, strings) are truthy.
    return 1;
}

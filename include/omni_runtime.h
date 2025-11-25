#ifndef OMNIKARAI_OMNI_RUNTIME_H
#define OMNIKARAI_OMNI_RUNTIME_H

// Forward declaration for dynamic type system (similar to Object from interpreter)
// We'll define a basic dynamic type system here for the compiled code.

typedef enum {
    OMNI_INTEGER,
    OMNI_BOOLEAN,
    OMNI_NIL,
    OMNI_STRING,
    // OMNI_FUNCTION, // Functions will likely be compiled to C functions directly
    // OMNI_RETURN_VALUE, // Handled by C return mechanism
} OmniType;

typedef struct OmniValue {
    OmniType type;
    union {
        long long integer;
        int boolean;
        char* string;
        // More types as needed
    } value;
} OmniValue;

// --- Runtime Functions ---
// For example, a print function that can handle different OmniValue types
void omni_print(OmniValue val);

// Functions to create OmniValue objects
OmniValue omni_new_integer(long long val);
OmniValue omni_new_boolean(int val);
OmniValue omni_new_nil();
OmniValue omni_new_string(const char* val);

// Basic binary operations (example for integers)
OmniValue omni_add(OmniValue left, OmniValue right);
OmniValue omni_subtract(OmniValue left, OmniValue right);
OmniValue omni_multiply(OmniValue left, OmniValue right);
OmniValue omni_divide(OmniValue left, OmniValue right);

// Comparison operations
OmniValue omni_equal(OmniValue left, OmniValue right);
OmniValue omni_not_equal(OmniValue left, OmniValue right);
OmniValue omni_less_than(OmniValue left, OmniValue right);
OmniValue omni_greater_than(OmniValue left, OmniValue right);
OmniValue omni_less_than_equal(OmniValue left, OmniValue right);
OmniValue omni_greater_than_equal(OmniValue left, OmniValue right);

// Helper for truthiness
int omni_is_truthy(OmniValue val);

#endif //OMNIKARAI_OMNI_RUNTIME_H

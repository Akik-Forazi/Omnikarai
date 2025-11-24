#ifndef OMNIKARAI_OBJECT_H
#define OMNIKARAI_OBJECT_H

// --- Object System ---
typedef enum {
    OBJ_INTEGER,
    OBJ_BOOLEAN,
    OBJ_NIL,
    OBJ_STRING,
} ObjectType;

typedef struct Object {
    ObjectType type;
    union {
        long long integer;
        int boolean;
        char* string;
    } value;
} Object;

#endif //OMNIKARAI_OBJECT_H

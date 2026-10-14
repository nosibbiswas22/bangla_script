#ifndef BANGLASCRIPT_OBJECT_H
#define BANGLASCRIPT_OBJECT_H

typedef enum {
    BS_OBJECT_NULL,
    BS_OBJECT_NUMBER,
    BS_OBJECT_BOOLEAN,
    BS_OBJECT_STRING,
    BS_OBJECT_FUNCTION,
    BS_OBJECT_INSTANCE
} BsObjectType;

typedef struct BsObject BsObject;
typedef struct BsClass BsClass;
typedef struct BsInstance BsInstance;

#endif

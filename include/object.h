#ifndef mt_object_h
#define mt_object_h

#include "common.h"
#include "value.h"
#include "table.h"
#include "chunk.h"

#define OBJ_TYPE(value)    (AS_OBJ(value)->type)

#define IS_BOUND_METHOD(value) isObjType(value, OBJ_BOUND_METHOD)
#define IS_CLASS(value)    isObjType(value, OBJ_CLASS)
#define IS_NATIVE_CLASS(value) isObjType(value, OBJ_NATIVE_CLASS)
#define IS_CLOSURE(value)  isObjType(value, OBJ_CLOSURE)
#define IS_STRING(value)   isObjType(value, OBJ_STRING)
#define IS_FUNCTION(value) isObjType(value, OBJ_FUNCTION)
#define IS_INSTANCE(value) isObjType(value, OBJ_INSTANCE)
#define IS_NATIVE(value)   isObjType(value, OBJ_NATIVE)
#define IS_LIST(value)     isObjType(value, OBJ_LIST)
#define IS_MODULE(value)   isObjType(value, OBJ_VALUE)

#define AS_BOUND_METHOD(value)  ((ObjBoundMethod*)AS_OBJ(value))
#define AS_CLASS(value)         ((ObjClass*)AS_OBJ(value))
#define AS_NATIVE_CLASS(value) ((ObjNativeClass*)AS_OBJ(value))
#define AS_CLOSURE(value)       ((ObjClosure*)AS_OBJ(value))
#define AS_FUNCTION(value)      ((ObjFunction*)AS_OBJ(value))
#define AS_INSTANCE(value)      ((ObjInstance*)AS_OBJ(value))
#define AS_NATIVE(value)        (((ObjNative*)AS_OBJ(value))->function)
#define AS_STRING(value)        ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value)       (((ObjString*)AS_OBJ(value))->chars)
#define AS_LIST(value)          ((ObjList*)AS_OBJ(value))
#define AS_MODULE(value)        ((ObjectModule*)AS_OBJ(value))

typedef enum
{
    OBJ_BOUND_METHOD,
    OBJ_CLASS,
    OBJ_NATIVE_CLASS,
    OBJ_CLOSURE,
    OBJ_FUNCTION,
    OBJ_INSTANCE,
    OBJ_NATIVE,
    OBJ_STRING,
    OBJ_LIST,
    OBJ_UPVALUE,
    OBJ_MODULE,
} ObjType;

struct sObj
{
	ObjType type;
	struct sObj* next;
};

typedef struct 
{
    Obj obj;
    int arity;
    int upvalueCount;
    Chunk chunk;
    ObjString* name;
} ObjFunction;

/* Shorthand */
typedef Value (*NativeFn)(int argCount, Value* args);

/*Native objects */
typedef struct 
{
    Obj obj;
    NativeFn function;
} ObjNative;

/* Create a native class for the module system */
typedef struct sObjNativeClass {
  Obj obj;
  ObjString* name;
  Table methods;
} ObjNativeClass;

/* Create strings that are fast */
struct sObjString
{
	Obj obj;
	int length;
	char * chars;
	uint32_t hash;
};

/* Used for detecting scope as a runtime object */
typedef struct ObjUpvalue
{
  Obj obj;
  Value* location; 
  Value closed;
  struct ObjUpvalue* next;
} ObjUpvalue;

/* such that variables from outer functions can be accesssed */
typedef struct
{
    Obj obj;
    ObjFunction* function;
    ObjUpvalue** upvalues;
    int upvalueCount;
} ObjClosure;

/* CLasses finally */
typedef struct 
{
    Obj obj;
    ObjString* name;
    Table methods;
} ObjClass;

/* Instances of classes */
typedef struct 
{
    Obj obj;
    ObjClass* klass;
    Table fields; // we hash the fields of the class
} ObjInstance;


/* Used to be bound methods of classes */
typedef struct 
{
  Obj obj;
  Value reciever;
  ObjClosure* method;
} ObjBoundMethod;

/* Adding lists to mt */
typedef struct 
{
    Obj obj;
    int count;
    int capacity;
    Value* items;
} ObjList;

/* Used for importing code */
typedef struct 
{
  Obj base;
  ObjString* path;
  ObjString* name;
  bool imported;
} ObjectModule;

ObjList* newList();
void appendToList(ObjList* list, Value value);
void storeToList(ObjList* list, int index, Value value);
Value indexFromList(ObjList* list, int index);
void deleteFromList(ObjList* list, int index);
bool isValidListIndex(ObjList* list, int index);
bool isValidStringIndex(ObjString* string, int index);
Value indexFromString(ObjString* string, int index);
ObjBoundMethod* newBoundMethod(Value reciever, ObjClosure* method);
ObjClass* newClass(ObjString* name);
ObjNativeClass *newNativeClass(ObjString *name);
ObjClosure* newClosure(ObjFunction* function);
ObjFunction* newFunction();
ObjInstance* newInstance(ObjClass* klass);
ObjNative* newNative(NativeFn functiom);
ObjString* takeString(char* chars, int length);
ObjString* copyString(const char* chars, int length);
ObjUpvalue* newUpvalue(Value* slot);

ObjString* fromCString(const char * chars);

ObjectModule* newModule(ObjString* path, ObjString* name);
ObjectModule* fromFullPath(const char *fullpath);

void printObject(Value value);

static inline bool isObjType(Value value, ObjType type)
{
	return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

/* Used for modules */
static inline ObjType getObjType(Value value) {
      return AS_OBJ(value)->type;
}

#endif

#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"
#include "vm.h"

#define ALLOCATE_OBJ(type, objectType) \
    (type*)allocateObject(sizeof(type), objectType)


static Obj* allocateObject(size_t size, ObjType type)
{
	Obj* object = (Obj*)reallocate(NULL, 0, size);
	object->type = type;

	object->next = vm.objects;
	vm.objects = object;
	
	return object;
}

/* Initialise a new list object */
ObjList* newList()
{
    ObjList* list = ALLOCATE_OBJ(ObjList, OBJ_LIST);
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
    return list;
}

/* Add a new value to the list */
void appendToList(ObjList* list, Value value) 
{
    if (list->capacity < list->count + 1) 
    {
        int oldCapacity = list->capacity;
        list->capacity = GROW_CAPACITY(oldCapacity);
        list->items = (Value*)reallocate(list->items, sizeof(Value)*oldCapacity, sizeof(Value)*list->capacity);
    }

    list->items[list->count] = value;
    list->count++;
    return;
}

/* Adds a value to s given place in a list */
void storeToList(ObjList* list, int index, Value value) 
{
    list->items[index] = value;
}

/* Get a value from a given index */
Value indexFromList(ObjList* list, int index) 
{
    return list->items[index];
}

/* Deletes an item from list */
void deleteFromList(ObjList* list, int index) 
{
    for (int i = 0; i < list->count - 1; i++) 
    {
        list->items[i] = list->items[i+1]; 
    }
    list->items[list->count - 1] = NIL_VAL;
    list->count--;
}

/* Check if its valid */
bool isValidListIndex(ObjList* list, int index) 
{
    if (index < 0 || index > list->count - 1) 
    {
        return false;
    }
    return true;
}

bool isValidStringIndex(ObjString* string, int index) {
    if (index < 0 || index > string->length - 1) {
        return false;
    }
    return true;
}

Value indexFromString(ObjString* string, int index) {
    ObjString* newString = copyString((char*)(string->chars + index), 1);
    return OBJ_VAL(newString);
}


/* Initialise a new object closure */
ObjClosure* newClosure(ObjFunction* function)
{
    ObjClosure* closure = ALLOCATE_OBJ(ObjClosure, OBJ_CLOSURE);
    closure->function = function;
    return closure;
}

/* Initialise a new function object */
ObjFunction* newFunction() 
{
    ObjFunction* function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);

    function->arity = 0;
    function->upvalueCount = 0;
    function->name = NULL;
    initChunk(&function->chunk);
    return function;
}

/* Create a new native function */
ObjNative* newNative(NativeFn function) 
{
    ObjNative* native = ALLOCATE_OBJ(ObjNative, OBJ_NATIVE);
    native->function = function;
    return native;
}

static ObjString* allocateString(char* chars, int length, uint32_t hash)
{
	ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
	string->length = length;
	string->chars = chars;
	string->hash = hash;

	tableSet(&vm.strings, string, NIL_VAL);
	
	return string;
}

/* FNV-1a hash function - could replace later with better hash */
static uint32_t hashString(const char * key, int length)
{
	uint32_t hash = 2166136261u;

	for (int i = 0; i < length; i++)
	{
		hash ^= key[i];
		hash *= 16777619;
	}

	return hash;
}

/* Take ownership of a string object */
ObjString* takeString(char* chars, int length)
{
	uint32_t hash = hashString(chars, length);

	ObjString* interned = tableFindString(&vm.strings, chars, length, hash);
	if (interned != NULL)
	{
		FREE_ARRAY(char, chars, length + 1);
		return interned;
	}
	
	return allocateString(chars, length, hash);
}

ObjString* copyString(const char * chars, int length)
{
	uint32_t hash = hashString(chars, length);
	ObjString* interned = tableFindString(&vm.strings, chars, length, hash);

	if (interned != NULL) return interned;
	
	char * heapChars = ALLOCATE(char, length+1);
	memcpy(heapChars, chars, length);
	heapChars[length] = '\0';

	return allocateString(heapChars, length, hash);
}

/* Print a function as a first class object */
static void printFunction(ObjFunction* function) 
{
    if (function->name == NULL) 
    {
        printf("<script>");
        return;
    }
    printf("<fn %s>", function->name->chars);
}

/* Print a list object */
static void printList(ObjList* list) 
{
    printf("[");
    for (int i = 0; i < list->count - 1; i++) 
    {
        printValue(list->items[i]);
        printf(", ");
    }
    if (list->count != 0) 
    {
        printValue(list->items[list->count - 1]);
    }
    printf("]");
}

void printObject(Value value)
{
    switch (OBJ_TYPE(value))
    {
    case OBJ_CLOSURE:
	printFunction(AS_CLOSURE(value)->function);
	break;
    case OBJ_FUNCTION:
        printFunction(AS_FUNCTION(value));
        break; 
    case OBJ_NATIVE:
        printf("<native fn>");
        break;
    case OBJ_STRING:
    	printf("%s", AS_CSTRING(value));
	    break;
    case OBJ_LIST:
        printList(AS_LIST(value));
        break;
	}
}

#include <stdlib.h>
#include <stdio.h>

#include "../include/memory.h"
#include "../include/vm.h"
#include "../include/iterator.h"

/* 
reallocate is the main function used by mt for many 
purposes:
|----------+----------+----------------------------|
| oldSize  | newSize  | Operation                  |
|----------+----------+----------------------------|
| 0        | Non-zero | Allocate new block         |
| Non-Zero | 0        | Free Allocation            |
| Non-Zero | <oldSize | Shrink existing allocation |
| Non-Zero | >oldSize | Grow existing allocation   |
|----------+----------+----------------------------|

The reason to use one function is to improve garbage collection
*/
void* reallocate(void* pointer, size_t oldSize, size_t newSize)
{
	/* Passing zero means free memory */
	if (newSize == 0)
	{
		free(pointer);
		return NULL;
	}

	/* realloc the correct amount of memory */
	void* result = realloc(pointer, newSize);
	/* Handle memory full or similar */
	if (result == NULL)
	{
		fprintf(stderr, "Error allocating memory...\n");
		exit(1);
	}
	
	return result;
}

/* freeObject is a method used to return the memory allocated 
 * by any of mt's internal objects such as stings and functions 
 * and others */
static void freeObject(Obj* object)
{
    switch (object->type)
    {

    case OBJ_BOUND_METHOD: 
    {
      FREE(ObjBoundMethod, object);
      break;  
    }
    case OBJ_FUNCTION: 
    {
        ObjFunction* function = (ObjFunction*)object;
        freeChunk(&function->chunk);
        FREE(ObjFunction, object);
        break;
    }

    case OBJ_LIST: 
    {
        FREE(ObjList, object);
        break;
    }

    case OBJ_TUPLE: 
    {
      FREE(ObjTuple, object);
      break;
    }

    case OBJ_ITERATOR: 
    {
      FREE(ObjectIterator, object);
      break;
    }

    case OBJ_CLASS: 
    {
        ObjClass* klass = (ObjClass*)object;
        freeTable(&klass->methods);
        FREE(ObjClass, object);
        break;
    }

    case OBJ_NATIVE_CLASS: 
    {
      ObjNativeClass* klass = (ObjNativeClass*)object;
      freeTable(&klass->methods);
      FREE(ObjNativeClass, object);
      break;
    }

    case OBJ_CLOSURE:
    {
    ObjClosure* closure = (ObjClosure*)object;
    FREE_ARRAY(ObjUpvalue*, closure->upvalues, closure->upvalueCount);
	FREE(ObjClosure, object);
	break;
    }

    case OBJ_INSTANCE: 
    {
        ObjInstance* instance = (ObjInstance*)object;
        freeTable(&instance->fields);
        FREE(ObjInstance, object);
        break;
    }
    
    case OBJ_NATIVE: 
    {
        FREE(ObjNative, object);
        break;
    }
	case OBJ_STRING: 
    {
		
		ObjString* string = (ObjString*)object;
		FREE_ARRAY(char, string->chars, string->length + 1);
		FREE(ObjString, object);
		break;
	}
  case OBJ_UPVALUE:
    FREE(ObjUpvalue, object);
    break;

    case OBJ_MODULE:
        FREE(ObjectModule, object);
        break;
	}
}

void freeObjects()
{
	Obj* object = vm.objects;
	while (object != NULL)
	{
		Obj* next = object->next;
		freeObject(object);
		object = next;
	}
}

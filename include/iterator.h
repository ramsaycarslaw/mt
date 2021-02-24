#ifndef mt_iterator_h
#define mt_iterator_h

#include "object.h"

#define IS_ITERATOR(value) isObjType(value, OBJ_ITERATOR) 
#define AS_ITERATOR(value) ((ObjectIterator*)AS_OBJ(value))

typedef struct 
{
  ObjList* list;
  int iter;
} ObjectIterator;

ObjectIterator* newIterator();
bool reachedEnd(ObjectIterator* iterable);
void advanceIterator(ObjectIterator* iterable);
Value valueFromIterable(ObjectIterator* iterable);

#endif

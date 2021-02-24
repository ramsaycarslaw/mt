#include <stdio.h>

#include "../include/iterator.h"
#include "../include/vm.h"
#include "../include/object.h"
#include "../include/memory.h"

static void printIterator() 
{
  printf("<iterable>");
}

ObjectIterator* newIterator() 
{
  ObjectIterator* iter = ALLOCATE(ObjectIterator, 1);
  return iter;
}

bool reachedEnd(ObjectIterator* iterable) 
{
  ObjList* list = iterable->list;
  if (list->count <= iterable->iter) {
    return true;
  }
  return false;
}

void advanceIterator(ObjectIterator* iterable) 
{
  iterable->iter = iterable->iter + 1;
}

Value valueFromIterable(ObjectIterator* iterable) 
{
  return iterable->list->items[iterable->iter];
}


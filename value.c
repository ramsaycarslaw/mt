#include <stdio.h>

#include "value.h"
#include "memory.h"

/* Initialise to a zero value */
void initValueArray(ValueArray* array)
{
	array->capacity = 0;
	array->count = 0;
	array->values = NULL;
}

/* Write a value to a value array */
void writeValueArray(ValueArray* array, Value value)
{
	if (array->capacity < array->count + 1)
	{
		int oldCapacity = array->capacity;
		array->capacity = GROW_CAPACITY(oldCapacity);
		array->values = GROW_ARRAY(Value, array->values,
					   oldCapacity, array->capacity);
	}

	array->values[array->count] = value;
	array->count++;
}

/* free a value array from memory */
void freeValueArray(ValueArray* array)
{
	FREE_ARRAY(Value, array->values, array->capacity);
	initValueArray(array);
}

void printValue(Value value)
{
  printf("%g", value);
}



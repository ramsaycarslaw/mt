#ifndef mt_value_h
#define mt_value_h

#include "common.h"

/* Abstract double so it can be changed without refactoring */
typedef double Value;

/* Another implemtation like chunk */
typedef struct {
	int capacity;
	int count;
	Value *values;
} ValueArray;

void initValueArray(ValueArray* array);
void writeValueArray(ValueArray* array, Value value);
void freeValueArray(ValueArray* array);
void printValue(Value value);

#endif

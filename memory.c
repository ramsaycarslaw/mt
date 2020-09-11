#include <stdlib.h>
#include <stdio.h>

#include "memory.h"

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

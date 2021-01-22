#ifndef mt_memory_h
#define mt_memory_h

/* Handles most memory processes for the mt Virtual Machine */

#include "common.h"
#include "object.h"

#define ALLOCATE(type, count) \
    (type*)reallocate(NULL, 0, sizeof(type) * (count))

#define FREE(type, pointer) reallocate(pointer, sizeof(type), 0)

/* Calculate the new capacity based on the current capacity using
new = old * 2 */
#define GROW_CAPACITY(capacity) \
    ((capacity) < 8 ? 8 : (capacity) * 2)

/* Grow the current array to be the same as specified capacity
   implements reallocate */
#define GROW_ARRAY(type, pointer, oldCount, newCount) \
    (type*)reallocate(pointer, sizeof(type) * (oldCount), \
        sizeof(type) * (newCount))

/* Make the current array free unused space */
#define FREE_ARRAY(type, pointer, oldCount) \
    reallocate(pointer, sizeof(type) * (oldCount), 0)

/* Plysically reallocate the memory */
void* reallocate(void* pointer, size_t oldSize, size_t newSize);
void freeObjects();

#endif

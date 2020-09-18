#include <stdio.h>

#include "native.h"

Value clockNative(int argCount, Value* args)
{
    return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}

Value readNative(int argCount, Value* args) 
{
	printf("%s", args[0].as);

    return OBJ_VAL(&args[0]);
}

#include "log.h"

#define RED     "\033[31m" 
#define RESET   "\033[0m"

/* Get the local time for printing as a log */
static char* getLocalTime() 
{
  // time.h stuff.
  time_t rawtime;
  struct tm * timeinfo;

  time( &rawtime );
  timeinfo = localtime (&rawtime);

  // be memory safe
  return  asctime ( timeinfo );
}

/* Print a single value with the time it happend */
static Value logPrintNative(int argCount, Value *args) 
{
  char * time = getLocalTime();

  if (argCount == 0) 
  {
    printf("%s\n", time);
  }

  if (argCount > 1) 
  {
    runtimeError("Too many arguments to 'log.Print', expected 1 got '%d%", argCount);
    printf("Perhaps you meant to use 'log.Printf'?\n");
    exit(74);
  }

  if (time[strlen(time) - 1] == '\n')
   time[strlen(time)-1] = '\0';
  
  printf("%s: %s\n", time, AS_CSTRING(args[0]));

  return NUMBER_VAL(0);
}

/* log.Fatal similar to golangs log.Fatal */
static Value logFatalNative(int argCount, Value *args) 
{
#ifndef _WIN32
  printf(RED);
#endif
  if (argCount != 1) 
  {
    runtimeError("Expected 1 argument to 'log.Fatal' got %d", argCount);
    exit(74);
  }

  // print the error
  printf("%s\n", AS_CSTRING(args[0]));

  // exit 
  exit(74);

#ifndef _WIN32
  printf(RESET);
#endif
}

/* Create a fake class for the log library  */
void createLogModule() 
{
  // name of the overall module
  ObjString* name = copyString("log", 3);
  push(OBJ_VAL(name));

  // we use the name to create the object
  ObjNativeClass *klass = newNativeClass(name);
  push(OBJ_VAL(name));

  defineModuleMethod(klass, "Print", logPrintNative);
  defineModuleMethod(klass, "Fatal", logFatalNative);

  tableSet(&vm.globals, name, OBJ_VAL(klass));
  pop();
  pop();
}

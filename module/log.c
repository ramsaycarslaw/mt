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

/* Print a progress bar to the screen takes a variable and a maximum value */
static Value logProgressNative(int argCount, Value *args) 
{
  // check arguments
  if (argCount < 2) 
  {
    runtimeError("Expected at least 2 arguments to 'log.Progress' got %d", argCount);
    exit(74);
  }

  // type errors
  if (!IS_NUMBER(args[0]) || !IS_NUMBER(args[1])) 
  {
    runtimeError("Expected values of type 'number' to 'log.Progress'.");
    exit(74);
  }

  long int length;

  // we have a length
  if (argCount == 3) 
  {
    length = (long int) AS_NUMBER(args[2]);
  } else {
    length = 20;
  }


  printf("\x1b[2K");

  int current = AS_NUMBER(args[0]);
  int max = AS_NUMBER(args[1]);

  if (current == max) {
    printf("\n");
    return NUMBER_VAL(0);
  }

  long double percent = (current / max) * 100;

  // print current percentage
  printf("%Lf%%", percent);

  // print start of prog bar
  printf(" [");

  long double step = length / max;
  for (int i = 0; i < round(step*current); i++)
    printf("#"); // █

  for (int i = round(current); i < round(max); i++) 
    printf(" ");

  printf("]");
  return NUMBER_VAL(0);

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

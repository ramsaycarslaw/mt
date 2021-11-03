#include "sorts.h"

static Value bubbleSortNative(int argCount, Value* args) {
  // bubbleSort(array)
  if (argCount != 1) {
    runtimeError("wrong number of arguments to 'sorts.Bubble'. got=%d, want=1", argCount);
    return NIL_VAL;
  }

  ObjList* list = AS_LIST(args[0]);

  for (int i = 0; i < list->count; i++) {
    for (int j = 0; j < list->count - i - 1; j++) {
      // check list->items[j] is a number
      if (!IS_NUMBER(list->items[j])) {
        runtimeError("sorts.Bubble: argument is not a number at index=%d", j);
        return NIL_VAL;
      }

      // do the same for j+1
      if (!IS_NUMBER(list->items[j + 1])) {
        runtimeError("sorts.Bubble: argument is not a number at index=%d", j + 1);
        return NIL_VAL;
      }

      if (AS_NUMBER(list->items[j]) > AS_NUMBER(list->items[j + 1])) {
        Value temp = list->items[j];
        list->items[j] = list->items[j + 1];
        list->items[j + 1] = temp;
      }
    }
  }

  return NIL_VAL;
}

// native insertion sort function
static Value insertionSortNative(int argCount, Value* args) {
  // insertionSort(array)
  if (argCount != 1) {
    runtimeError("wrong number of arguments to 'sorts.Insertion'. got=%d, want=1", argCount);
    return NIL_VAL;
  }

  ObjList* list = AS_LIST(args[0]);

  for (int i = 1; i < list->count; i++) {
    Value temp = list->items[i];
    int j = i - 1;

    while (j >= 0 && AS_NUMBER(list->items[j]) > AS_NUMBER(temp)) {
      list->items[j + 1] = list->items[j];
      j--;
    }

    list->items[j + 1] = temp;
  }

  return NIL_VAL;
}

// partition for quicksort
static int partition(Value* arr, int left, int right) {
  Value pivot = arr[right];
  int i = left - 1;

  for (int j = left; j <= right - 1; j++) {
    if (AS_NUMBER(arr[j]) <= AS_NUMBER(pivot)) {
      i++;
      Value temp = arr[i];
      arr[i] = arr[j];
      arr[j] = temp;
    }
  }

  Value temp = arr[i + 1];
  arr[i + 1] = arr[right];
  arr[right] = temp;

  return i + 1;
}

// quicksort function
static void quickSort(Value* items, int left, int right) {
  if (left < right) {
    int pivotIndex = partition(items, left, right);
    quickSort(items, left, pivotIndex - 1);
    quickSort(items, pivotIndex + 1, right);
  }
}

// function to quick sort an array
static Value quickSortNative(int argCount, Value* args) {
  // quickSort(array)
  if (argCount != 1) {
    runtimeError("wrong number of arguments to 'sorts.Quick'. got=%d, want=1", argCount);
    return NIL_VAL;
  }

  ObjList* list = AS_LIST(args[0]);

  quickSort(list->items, 0, list->count - 1);

  return NIL_VAL;
}

/* Finally we create the module */
void createSortsModule() 
{
  // name of the overall module
  ObjString* name = copyString("sorts", 5);
  push(OBJ_VAL(name));


  // we use the name to create the object
  ObjNativeClass *klass = newNativeClass(name);
  push(OBJ_VAL(name));

  defineModuleMethod(klass, "Bubble", bubbleSortNative);
  defineModuleMethod(klass, "Quick", quickSortNative);
  defineModuleMethod(klass, "Sort", quickSortNative);
  defineModuleMethod(klass, "Insertion", insertionSortNative);

  tableSet(&vm.globals, name, OBJ_VAL(klass));
  pop();
  pop();
}

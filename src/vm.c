#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/common.h"
#include "../include/compiler.h"
#include "../include/debug.h"
#include "../include/memory.h"
#include "../include/native.h"
#include "../include/object.h"
#include "../include/vm.h"

/* Maybe take a pointer later to remove the global variable */
VM vm;

/*
Since the stack array is declared directly inline in the VM struct, we don’t
need to allocate it. We don’t even need to clear the unused cells in the
array—we simply won’t access them until after values have been stored in them.
The only initialization we need is to set stackTop to point to the beginning of
the array to indicate that the stack is empty.
*/

static void resetStack() {
  vm.stackTop = vm.stack;
  vm.frameCount = 0;
  vm.openUpvalues = NULL;
}

static void runtimeError(const char *format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputs("\n", stderr);

  for (int i = vm.frameCount - 1; i >= 0; i--) {
    CallFrame *frame = &vm.frames[i];
    ObjFunction *function = frame->closure->function;

    size_t instruction = frame->ip - function->chunk.code - 1;
    fprintf(stderr, "[line %d] in ", function->chunk.lines[instruction]);

    if (function->name == NULL) {
      fprintf(stderr, "script\n");
    } else {
      fprintf(stderr, "%s()\n", function->name->chars);
    }
  }

  resetStack();
}

/* define a new built in function */
static void defineNative(const char *name, NativeFn function) {
  push(OBJ_VAL(copyString(name, (int)strlen(name))));
  push(OBJ_VAL(newNative(function)));
  tableSet(&vm.globals, AS_STRING(vm.stack[0]), vm.stack[1]);
  pop();
  pop();
}

void initVM() {
  resetStack();
  vm.objects = NULL;
  initTable(&vm.strings);
  initTable(&vm.globals);

  vm.initString = NULL;
  vm.initString = copyString("init", 4);

  /* System */
  defineNative("clock", clockNative);
  defineNative("sleep", sleepNative);
  defineNative("read", readNative);
  defineNative("write", writeNative);
  defineNative("randInt", randIntNative);
  defineNative("input", inputNative);
  defineNative("number", doubleNative);
  defineNative("string", stringNative);
  defineNative("exit", exitNative);
  defineNative("clear", clearNative);
  defineNative("show", showNative);
  defineNative("Cd", cdNative);

  /* Printing */
  defineNative("printf", printfNative);
  defineNative("println", printlnNative);
  defineNative("color", colorSetNative);
  defineNative("bg", bgSetNative);

  /* Arrays */
  defineNative("append", appendNative);
  defineNative("delete", deleteNative);
  defineNative("len", lenNative);
}

void freeVM() {
  freeTable(&vm.globals);
  freeTable(&vm.strings);
  vm.initString = NULL;
  freeObjects();
}

/* wrapper for getting next value in call stack */
static Value peek(int distance) { return vm.stackTop[-1 - distance]; }

/* calls a function */
static bool call(ObjClosure *closure, int argCount) {
  // Handle errors
  if (argCount != closure->function->arity) {
    runtimeError("Expected %d arguments but got %d.", closure->function->arity,
                 argCount);
    return false;
  }

  if (vm.frameCount == FRAMES_MAX) {
    runtimeError("Stack overflow.");
    return false;
  }

  CallFrame *frame = &vm.frames[vm.frameCount++];
  frame->closure = closure;
  frame->ip = closure->function->chunk.code;

  frame->slots = vm.stackTop - argCount - 1;
  return true;
}

/* gets the value of function body to call */
static bool callValue(Value callee, int argCount) {
  if (IS_OBJ(callee)) {
    switch (OBJ_TYPE(callee)) {

    case OBJ_BOUND_METHOD: {
      ObjBoundMethod *bound = AS_BOUND_METHOD(callee);
      vm.stackTop[-argCount - 1] = bound->reciever;
      return call(bound->method, argCount);
    }

    case OBJ_CLASS: {
      ObjClass *klass = AS_CLASS(callee);
      vm.stackTop[-argCount - 1] = OBJ_VAL(newInstance(klass));
      Value initializer;
      if (tableGet(&klass->methods, vm.initString, &initializer)) {
        return call(AS_CLOSURE(initializer), argCount);
      } else if (argCount != 0) {
        runtimeError("Expected 0 arguments but got %d.", argCount);
        return false;
      }
      return true;
    }
    case OBJ_CLOSURE:
      return call(AS_CLOSURE(callee), argCount);
    case OBJ_FUNCTION:
      return call(AS_CLOSURE(callee), argCount);
    case OBJ_NATIVE: {
      NativeFn native = AS_NATIVE(callee);
      Value result = native(argCount, vm.stackTop - argCount);
      vm.stackTop -= argCount + 1;
      push(result);
      return true;
    }
    default:
      break;
    }
  }
  runtimeError("Can only call functions and classes.");
  return false;
}

/* Same as invoke but from class */
static bool invokeFromClass(ObjClass *klass, ObjString *name, int argCount) {
  Value method;
  if (!tableGet(&klass->methods, name, &method)) {
    runtimeError("Undefined property '%s'.", name->chars);
    return false;
  }

  return call(AS_CLOSURE(method), argCount);
}

/* Invoke methods optimiser */
static bool invoke(ObjString *name, int argCount) {
  Value receiver = peek(argCount);

  if (!IS_INSTANCE(receiver)) {
    runtimeError("Only instances have methods");
    return false;
  }

  ObjInstance *instance = AS_INSTANCE(receiver);

  Value value;
  if (tableGet(&instance->fields, name, &value)) {
    vm.stackTop[-argCount - 1] = value;
    return callValue(value, argCount);
  }

  return invokeFromClass(instance->klass, name, argCount);
}

/* Runtime code for bound methods of classes */
static bool bindMethod(ObjClass *klass, ObjString *name) {
  Value method;
  if (!tableGet(&klass->methods, name, &method)) {
    runtimeError("Undefined property '%s'.", name->chars);
    return false;
  }
  ObjBoundMethod *bound = newBoundMethod(peek(0), AS_CLOSURE(method));

  pop();
  push(OBJ_VAL(bound));
  return true;
}

static ObjUpvalue *captureUpvalue(Value *local) {
  ObjUpvalue *prevUpvalue = NULL;
  ObjUpvalue *upvalue = vm.openUpvalues;

  while (upvalue != NULL && upvalue->location > local) {
    prevUpvalue = upvalue;
    upvalue = upvalue->next;
  }

  if (upvalue != NULL && upvalue->location == local) {
    return upvalue;
  }

  ObjUpvalue *createdUpvalue = newUpvalue(local);

  createdUpvalue->next = upvalue;

  if (prevUpvalue == NULL) {
    vm.openUpvalues = createdUpvalue;
  } else {
    prevUpvalue->next = createdUpvalue;
  }

  return createdUpvalue;
}

static void closeUpvalues(Value *last) {
  while (vm.openUpvalues != NULL && vm.openUpvalues->location >= last) {
    ObjUpvalue *upvalue = vm.openUpvalues;
    upvalue->closed = *upvalue->location;
    upvalue->location = &upvalue->closed;
    vm.openUpvalues = upvalue->next;
  }
}

static void defineMethod(ObjString *name) {
  Value method = peek(0);
  ObjClass *klass = AS_CLASS(peek(1));
  tableSet(&klass->methods, name, method);
  pop();
}

static bool isFalsey(Value value) {
  return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static void concatenate() {
  ObjString *b = AS_STRING(pop());
  ObjString *a = AS_STRING(pop());

  int length = a->length + b->length;
  char *chars = ALLOCATE(char, length + 1);
  memcpy(chars, a->chars, a->length);
  memcpy(chars + a->length, b->chars, b->length);
  chars[length] = '\0';

  ObjString *result = takeString(chars, length);
  push(OBJ_VAL(result));
}

static int run() {
  CallFrame *frame = &vm.frames[vm.frameCount - 1];
#define READ_BYTE() (*frame->ip++) // method to get the next byte
#define READ_CONSTANT()                                                        \
  (frame->closure->function->chunk.constants.values[READ_BYTE()])
#define READ_SHORT()                                                           \
  (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))
#define READ_STRING() AS_STRING(READ_CONSTANT())
#define BINARY_OP(valueType, op)                                               \
  do {                                                                         \
    if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) {                          \
      runtimeError("Operands must be numbers.");                               \
      return INTERPRET_RUNTIME_ERROR;                                          \
    }                                                                          \
    double b = AS_NUMBER(pop());                                               \
    double a = AS_NUMBER(pop());                                               \
    push(valueType(a op b));                                                   \
  } while (false)

  for (;;) {
#ifdef MT_DEBUG_TRACE_EXEC
    printf("       ");
    for (Value *slot = vm.stack; slot < vm.stackTop; slot++) {
      printf("[ ");
      printValue(*slot);
      printf(" ]");
    }
    printf("\n");
    disassembleInstruction(
        &frame->closure->function->chunk,
        (int)(frame->ip - frame->closure->function->chunk.code));
#endif
    uint8_t instruction;
    switch (instruction = READ_BYTE()) {
    case OP_CONSTANT: {
      Value constant = READ_CONSTANT();
      push(constant);
      break;
    }

    case OP_NIL:
      push(NIL_VAL);
      break;
    case OP_TRUE:
      push(BOOL_VAL(true));
      break;
    case OP_FALSE:
      push(BOOL_VAL(false));
      break;

    case OP_POP:
      pop();
      break;

    case OP_GET_LOCAL: {
      uint8_t slot = READ_BYTE();
      push(frame->slots[slot]);
      break;
    }

    case OP_SET_LOCAL: {
      uint8_t slot = READ_BYTE();
      frame->slots[slot] = peek(0);
      break;
    }

    case OP_GET_GLOBAL: {
      ObjString *name = READ_STRING();
      Value value;

      if (!tableGet(&vm.globals, name, &value)) {
        runtimeError("Undefined variable '%s'.", name->chars);
        return INTERPRET_RUNTIME_ERROR;
      }
      push(value);
      break;
    }

    case OP_DEFINE_GLOBAL: {
      ObjString *name = READ_STRING();
      tableSet(&vm.globals, name, peek(0));
      pop();
      break;
    }

    case OP_SET_GLOBAL: {
      ObjString *name = READ_STRING();
      if (tableSet(&vm.globals, name, peek(0))) {
        tableDelete(&vm.globals, name);
        runtimeError("Undefined variable '%s'.", name->chars);
        return INTERPRET_RUNTIME_ERROR;
      }
      break;
    }

    case OP_GET_UPVALUE: {
      uint8_t slot = READ_BYTE();
      push(*frame->closure->upvalues[slot]->location);
      break;
    }

    case OP_SET_UPVALUE: {
      uint8_t slot = READ_BYTE();
      *frame->closure->upvalues[slot]->location = peek(0);
      break;
    }

    case OP_GET_PROPERTY: {
      if (!IS_INSTANCE(peek(0))) {
        runtimeError("Only instances have properties.");
        return INTERPRET_RUNTIME_ERROR;
      }
      ObjInstance *instance = AS_INSTANCE(peek(0));
      ObjString *name = READ_STRING();

      Value value;
      if (tableGet(&instance->fields, name, &value)) {
        pop();
        push(value);
        break;
      }

      if (!bindMethod(instance->klass, name)) {
        return INTERPRET_RUNTIME_ERROR;
      }
      break;
    }

    case OP_SET_PROPERTY: {
      if (!IS_INSTANCE(peek(1))) {
        runtimeError("Only instances have fields.");
        return INTERPRET_RUNTIME_ERROR;
      }
      ObjInstance *instance = AS_INSTANCE(peek(1));
      tableSet(&instance->fields, READ_STRING(), peek(0));

      Value value = pop();
      pop();
      push(value);
      break;
    }

    case OP_GET_SUPER: {
      ObjString* name = READ_STRING();
      ObjClass* superclass = AS_CLASS(pop());
      if (!bindMethod(superclass, name)) {
        return INTERPRET_RUNTIME_ERROR;
      }
      break;
    }

    case OP_EQUAL: {
      Value b = pop();
      Value a = pop();
      push(BOOL_VAL(valuesEqual(a, b)));
      break;
    }

    case OP_GREATER:
      BINARY_OP(BOOL_VAL, >);
      break;
    case OP_LESS:
      BINARY_OP(BOOL_VAL, <);
      break;
    case OP_ADD: {
      if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
        concatenate();
      } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
        double b = AS_NUMBER(pop());
        double a = AS_NUMBER(pop());
        push(NUMBER_VAL(a + b));
      } else if (IS_LIST(peek(0)) && IS_NUMBER(peek(1))) {
        ObjList *list = AS_LIST(pop());
        double b = AS_NUMBER(pop());

        for (int i = 0; i < list->count; i++) {
          if (!IS_NUMBER(list->items[i])) {
            continue;
          }
          list->items[i] = NUMBER_VAL(AS_NUMBER(list->items[i]) + b);
        }
        push(OBJ_VAL(list));
      } else if (IS_NUMBER(peek(0)) && IS_LIST(peek(1))) {
        double b = AS_NUMBER(pop());
        ObjList *list = AS_LIST(pop());

        for (int i = 0; i < list->count; i++) {
          if (!IS_NUMBER(list->items[i])) {
            continue;
          }
          list->items[i] = NUMBER_VAL(AS_NUMBER(list->items[i]) + b);
        }
        push(OBJ_VAL(list));
      } else {
        runtimeError("Operands must be two numbers or two strings.");
        return INTERPRET_RUNTIME_ERROR;
      }
      break;
    }
    case OP_SUBTRACT:
      if (IS_LIST(peek(0)) && IS_NUMBER(peek(1))) {
        ObjList *list = AS_LIST(pop());
        double b = AS_NUMBER(pop());

        for (int i = 0; i < list->count; i++) {
          if (!IS_NUMBER(list->items[i])) {
            continue;
          }
          list->items[i] = NUMBER_VAL(AS_NUMBER(list->items[i]) - b);
        }
        push(OBJ_VAL(list));
      } else if (IS_NUMBER(peek(0)) && IS_LIST(peek(1))) {
        double b = AS_NUMBER(pop());
        ObjList *list = AS_LIST(pop());

        for (int i = 0; i < list->count; i++) {
          if (!IS_NUMBER(list->items[i])) {
            continue;
          }
          list->items[i] = NUMBER_VAL(AS_NUMBER(list->items[i]) - b);
        }
        push(OBJ_VAL(list));
      } else {
        BINARY_OP(NUMBER_VAL, -);
      }
      break;
    case OP_MULTIPLY:
      if (IS_LIST(peek(0)) && IS_NUMBER(peek(1))) {
        ObjList *list = AS_LIST(pop());
        double b = AS_NUMBER(pop());

        for (int i = 0; i < list->count; i++) {
          if (!IS_NUMBER(list->items[i])) {
            continue;
          }
          list->items[i] = NUMBER_VAL(AS_NUMBER(list->items[i]) * b);
        }
        push(OBJ_VAL(list));
      } else if (IS_NUMBER(peek(0)) && IS_LIST(peek(1))) {
        double b = AS_NUMBER(pop());
        ObjList *list = AS_LIST(pop());

        for (int i = 0; i < list->count; i++) {
          if (!IS_NUMBER(list->items[i])) {
            continue;
          }
          list->items[i] = NUMBER_VAL(AS_NUMBER(list->items[i]) * b);
        }
        push(OBJ_VAL(list));
      } else {
        BINARY_OP(NUMBER_VAL, *);
      }
      break;
    case OP_DIVIDE:
      if (IS_LIST(peek(0)) && IS_NUMBER(peek(1))) {
        ObjList *list = AS_LIST(pop());
        double b = AS_NUMBER(pop());

        for (int i = 0; i < list->count; i++) {
          if (!IS_NUMBER(list->items[i])) {
            continue;
          }
          list->items[i] = NUMBER_VAL(AS_NUMBER(list->items[i]) / b);
        }
        push(OBJ_VAL(list));
      } else if (IS_NUMBER(peek(0)) && IS_LIST(peek(1))) {
        double b = AS_NUMBER(pop());
        ObjList *list = AS_LIST(pop());

        for (int i = 0; i < list->count; i++) {
          if (!IS_NUMBER(list->items[i])) {
            continue;
          }
          list->items[i] = NUMBER_VAL(AS_NUMBER(list->items[i]) / b);
        }
        push(OBJ_VAL(list));
      } else {
        BINARY_OP(NUMBER_VAL, /);
      }
      break;

    case OP_NOT:
      push(BOOL_VAL(isFalsey(pop())));
      break;
    case OP_POW: {
      if (IS_LIST(peek(0)) && IS_NUMBER(peek(1))) {
        ObjList *list = AS_LIST(pop());
        double b = AS_NUMBER(pop());

        for (int i = 0; i < list->count; i++) {
          if (!IS_NUMBER(list->items[i])) {
            continue;
          }
          list->items[i] = NUMBER_VAL(pow(AS_NUMBER(list->items[i]), b));
        }
        push(OBJ_VAL(list));
      } else if (IS_NUMBER(peek(0)) && IS_LIST(peek(1))) {
        double b = AS_NUMBER(pop());
        ObjList *list = AS_LIST(pop());

        for (int i = 0; i < list->count; i++) {
          if (!IS_NUMBER(list->items[i])) {
            continue;
          }
          list->items[i] = NUMBER_VAL(pow(AS_NUMBER(list->items[i]), b));
        }
        push(OBJ_VAL(list));
      } else {
        double b = AS_NUMBER(pop());
        double a = AS_NUMBER(pop());
        push(NUMBER_VAL(pow(a, b)));
      }
      break;
    }
    case OP_MOD: {
      if (!(IS_NUMBER(peek(0)) && IS_NUMBER(peek(1)))) {
        runtimeError("Operands must be numbers.");
      }
      double b = AS_NUMBER(pop());
      double a = AS_NUMBER(pop());
      double c = (long int)a % (long int)b;
      push(NUMBER_VAL(c));
      break;
    }
    case OP_NEGATE: {
      if (!IS_NUMBER(peek(0))) {
        runtimeError("Operand must be a number.");
        return INTERPRET_RUNTIME_ERROR;
      }

      push(NUMBER_VAL(-AS_NUMBER(pop())));
      break;
    }
    case OP_INCR: {
      if (!IS_NUMBER(peek(0))) {
        runtimeError("Operand must be a number.");
        return INTERPRET_RUNTIME_ERROR;
      }

      push(NUMBER_VAL(AS_NUMBER(pop()) + 1));
      break;
    }
    case OP_PRINT: {
      printValue(pop());
      printf("\n");
      break;
    }

    case OP_USE: {
      /* This has already been handlled by the preprocessor
       * so there is no need to do anything here */
      break;
    }

    case OP_JUMP: {
      uint16_t offset = READ_SHORT();
      frame->ip += offset;
      break;
    }

    case OP_JUMP_IF_FALSE: {
      uint16_t offset = READ_SHORT();
      if (isFalsey(peek(0)))
        frame->ip += offset;
      break;
    }

    case OP_LOOP: {
      uint16_t offset = READ_SHORT();
      frame->ip -= offset;
      break;
    }

    case OP_CALL: {
      int argCount = READ_BYTE();
      if (!callValue(peek(argCount), argCount)) {
        return INTERPRET_RUNTIME_ERROR;
      }

      frame = &vm.frames[vm.frameCount - 1];
      break;
    }

    case OP_CLOSURE: {
      ObjFunction *function = AS_FUNCTION(READ_CONSTANT());
      ObjClosure *closure = newClosure(function);
      push(OBJ_VAL(closure));
      for (int i = 0; i < closure->upvalueCount; i++) {
        uint8_t isLocal = READ_BYTE();
        uint8_t index = READ_BYTE();
        if (isLocal) {
          closure->upvalues[i] = captureUpvalue(frame->slots + index);
        } else {
          closure->upvalues[i] = frame->closure->upvalues[index];
        }
      }
      break;
    }

    case OP_INVOKE: {
      ObjString *method = READ_STRING();
      int argCount = READ_BYTE();
      if (!invoke(method, argCount)) {
        return INTERPRET_RUNTIME_ERROR;
      }
      frame = &vm.frames[vm.frameCount - 1];
      break;
    }

    case OP_SUPER_INVOKE: 
    {
      ObjString* method = READ_STRING();
      int argCount = READ_BYTE();
      ObjClass* superclass = AS_CLASS(pop());
      if (!invokeFromClass(superclass, method, argCount)) 
      {
        return INTERPRET_RUNTIME_ERROR;
      }
      frame = &vm.frames[vm.frameCount - 1];
      break;                      
    }

    case OP_BUILD_LIST: {
      ObjList *list = newList();
      uint8_t itemCount = READ_BYTE();

      push(OBJ_VAL(list));
      for (int i = itemCount; i > 0; i--) {
        appendToList(list, peek(i));
      }
      pop();

      while (itemCount-- > 0) {
        pop();
      }

      push(OBJ_VAL(list));
      break;
    }
    
    case OP_GENERATE_LIST: {
      ObjList *list = newList();                       
      uint8_t max = READ_BYTE();

      push(OBJ_VAL(list));

      for (int i = max-1; i >= 0; i--) {
        appendToList(list, NUMBER_VAL(i));
      }  
      pop();
      pop();

      push(OBJ_VAL(list));
      break;
    }

    case OP_INDEX_SUBSCR: {
      Value index = pop();
      Value indexable = pop();
      Value result;

      if (IS_LIST(indexable)) {
        ObjList *list = AS_LIST(indexable);

        if (!IS_NUMBER(index)) {
          runtimeError("List index is not a number.");
          return INTERPRET_RUNTIME_ERROR;
        } else if (!isValidListIndex(list, AS_NUMBER(index))) {
          runtimeError("List index out of range.");
          return INTERPRET_RUNTIME_ERROR;
        }
        result = indexFromList(list, AS_NUMBER(index));
      } else if (IS_STRING(indexable)) {
        ObjString *string = AS_STRING(indexable);
        if (!IS_NUMBER(index)) {
          runtimeError("String index is not a number");
          return INTERPRET_RUNTIME_ERROR;
        } else if (!isValidStringIndex(string, AS_NUMBER(index))) {
          runtimeError("String index out of range");
          return INTERPRET_RUNTIME_ERROR;
        }
        result = indexFromString(string, AS_NUMBER(index));
      } else {
        runtimeError("Object is not indexable");
        return INTERPRET_RUNTIME_ERROR;
      }

      push(result);
      break;
    }

    case OP_STORE_SUBSCR: {
      Value item = pop();
      Value index = pop();
      Value indexable = pop();

      if (!IS_LIST(indexable)) {
        runtimeError("Cannot store value in a non-list.");
        return INTERPRET_RUNTIME_ERROR;
      }
      ObjList *list = AS_LIST(indexable);

      if (!IS_NUMBER(index)) {
        runtimeError("List index is not a number.");
        return INTERPRET_RUNTIME_ERROR;
      }
      int index_i = AS_NUMBER(index);

      if (!isValidListIndex(list, index_i)) {
        runtimeError("Invalid list index.");
        return INTERPRET_RUNTIME_ERROR;
      }

      storeToList(list, index_i, item);
      push(item);
      break;
    }

    case OP_CLOSE_UPVALUE: {
      closeUpvalues(vm.stackTop - 1);
      pop();
      break;
    }

    case OP_RETURN: {
      Value result = pop();

      closeUpvalues(frame->slots);

      vm.frameCount--;
      if (vm.frameCount == 0) {
        pop();
        return INTERPRET_OK;
      }

      vm.stackTop = frame->slots;
      push(result);

      frame = &vm.frames[vm.frameCount - 1];
      break;
    }

    case OP_CLASS:
      push(OBJ_VAL(newClass(READ_STRING())));
      break;

    // inherit from superclass
    case OP_INHERIT: {
      Value superclass = peek(1);                 
      
      /* Stop users from inheriting from a class that doesn't exist */
      if (!IS_CLASS(superclass)) {
        runtimeError("Superclass must be a class.");
        return INTERPRET_RUNTIME_ERROR;
      }

      ObjClass* subclass = AS_CLASS(peek(0));
      tableAddAll(&AS_CLASS(superclass)->methods, &subclass->methods);

      pop(); // subclass
      break;
    }

    case OP_COPY:
      push(peek(0));
      break;

    case OP_METHOD:
      defineMethod(READ_STRING());
      break;
    }
  }
#undef READ_BYTE
#undef READ_CONSTANT
#undef READ_SHORT
#undef READ_STRING
#undef BINARY_OP
}

InterpretResult interpret(const char *source) {
  ObjFunction *function = compile(source);
  if (function == NULL)
    return INTERPRET_COMPILE_ERROR;

  push(OBJ_VAL(function));

  ObjClosure *closure = newClosure(function);
  pop();
  push(OBJ_VAL(closure));
  callValue(OBJ_VAL(closure), 0);

  return run();
}

void push(Value value) {
  *vm.stackTop = value;
  vm.stackTop++;
}

Value pop() {
  vm.stackTop--;
  return *vm.stackTop;
}

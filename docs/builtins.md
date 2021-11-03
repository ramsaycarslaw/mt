Built In Functions
====================

The `mt` programming language contains a handful of builtin functions to aid with common operations.

## Timing

There are two functions built in to aid with keeping track of time, they are `clock()` and `sleep(n)`.

### The Clock function

The clock function can be used to time the execution of a function like below

```rust
fn funcToTime() {
  for (var i = 0; i < 100; i += 1) {
    print i;
  }
}

let start = clock();
funcToTime();
let elapsed = clock() - start;

print "Time taken " + string(elapsed);
```

### The Sleep Function

The sleep function takes an argument in seconds and pauses execution of the program for that amount of time.

```python
print "Hello";
sleep(10);
print "World";
```



# Handelling logical and type errors with the `assert` and `errors` modules.


A common source of error in dynamically typed programming languages is user defined functions being passed values of the wrong type.
To avoid this `MT` provides a module called `assert`.
Assert can be used to verify equatity and other boolean values and verify types.

## Type Errors

Consider the following function that adds two integers and returns thier result.

```rust
fn add(x, y) 
{
  return x+y;
}
```

If it is left as it is then a user may do something like the following. Although this seems like an obvious error, large functions are often not as clear.

```rust
add(true, false);
```

Even more confusing than the last case is something like the following, becuase `MT` uses `+` for string concateation the call below will not throw an error.

```rust
add("1", "2");  // returns "12"
```

Theese can cause a runtime error in the `MT` virtual machine. To avoid this we may use the function `assert.Number(args...)`, this function takes `n` parameters and returns true if and only if all the values are or can be interpreted as numbers.
The safe function will look something like this.

### Solution

We use the 'assert.Number' function to avoid type errors. This will definatley throe a well defined error to the user to let them know they have passed the wrong type.

```rust
fn add(x, y) {
  assert.Number(x, y);
  
  return x + y;
}
```


## Logical Errors

Take the following function which approximates the square root of a function. 
It is well know that `sqrt(x)` is undefinded for all `x < 0`. To check for this is simple:

```rust
fn sqrt(x) {
  if (x < 0) {
    // do something
  }
  
  // compute square root
}
```

What is not so obvious is what value to return? -1, nil? Any of those could break code calling that function if it expects a positive integer. To let the user know an illeagl value has been passed we can use `errors.Raise` to throw a runtime error which will let any user know they have broken the function.
This is much less ambigous then returning nil or some other none breaking code.

```rust
fn sqrt(x) {
  if (x < 0) {
    errors.Raise("Cannot compute square root of values less than 0.");
  }
  
  // compute square root
}
```

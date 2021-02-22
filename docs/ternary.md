The Ternary Operator
====================

The ternary operator is syntax borrowed from C. It is a syntax for reducing a common pattern of if statement to one line.
Consider the following pattern:

```rust
if (condition == true) {
  doSomething();
} else {
  doSomethingElse();
}
```

The ternary operator allows you to reduce that code to the following:

```rust
condition ? doSomething() : doSomethingElse();
```

If the condition is true then it will run `doSomething()` if i is false it will run `doSomethingElse()`. 
It doesn't just have to be function calls either, you can put any statement in a ternary operator.

## Simple Example
Below is an example that checks if a user is old enough.

```rust
var age = number(input("Enter your age: "));

print ((age >= 18) ? "Allowed in" : "Not allowed");
```

The ternary operator should be thought of as a shorthand not a new concept.

## Common Uses

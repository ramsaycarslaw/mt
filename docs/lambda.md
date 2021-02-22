# Lambda Expressions

Lambda expressions or anonymous functions can be used when there is no need for a function to be declared with a name. Lambda's in `MT` draw inspiration from Haskell but try to look consistent with the rest of the language.

Syntax
------
The syntax for Lambdas is as follows. Given the lambda is still a variable assignment the `;` token at the end is still needed.

```
var foo = \ -> {
  print "Hello, World!";
};
```

Or it can be condensed onto one line like so

```
var foo = \ -> { print "Hello, World!"; };
```

You can call a lambda function the same way you would any other function.

```
foo();
```

Which gives:

```
$ Hello, World!
```

Arguments Syntax
----------------

Much like other functions you can also pass arguments to lambda expressions. The syntax for which is as follows.

```
var addOne = \x -> { return x += 1; };

print addOne(10);
```

Running this gives:

```
$ 11
```

You can have up to 256 arguments in a Lambda expression. Lambda expressions follow all the same rules for closures and local variables as named functions so there is no need to worry about memory.


Inline Lambdas
--------------

There is no need to assign a lambda to a variable, the following is perfectly valid.

```
print (\x -> {return x + 1;})(2);  // prints 3
```

Higher Order Functions
----------------------

`MT` supports the use of higher order functions through the use of Lambda functions, in the example below we can change the function passed to the higher order function and change the behaviour of the function.

```
var higherOrderFunc = \x, func -> {
  return x + func(x);
};

print higherOrderFunc(2, \x -> {return x * x; });  // prints 6

print higherOrderFunc(2, \x -> {return x + 3; });  // prints 7
```

Use Cases
---------

If you are familiar with Haskell you will probably recognise the following.

```haskell
fib :: Int -> Int
fib 0 = 0
fib 1 = 1
fib x = fib (x-1) + fib (x-2)
```

In `MT` we can achive similar using switch/case statements and Lambda functions

```
var fib = 
\n -> { switch (n) {
    case 0:       return 0;
    case 1:       return 1;
    default:      return fib (n-1) + fib (n - 2);
}};
```


Lambda functions can be used to make function definitions more clear and create shorthands for the programmer.



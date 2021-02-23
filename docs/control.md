Control Flow in `MT`
==================

`MT` provides several methods to control the execution of the program. C programmer's will find it familiar whereas anyone coming from a Python style language should familiarise themselves with the definitions below.


Overview
------

* If/Else

```c
if (password == "1234") {
  print "Not Secure!";
} else if (password == "password") {
  print "Not very secure";
} else {
  print "Probably okay";
}
```

* Switch

```c
switch (age) {
  ...
  case 16:
    print "not old enough to drive or vote";
  case 17:
    print "can drive";
  case 18:
    print "can vote";
  default:
    print "can drive and vote";
}
```


* Ternary

Ternary operators are not covered in this file, see [here](./ternary.md) for more information.

```c
print (boolean ? "True" : "False");
```

If/Else
-------

The if syntax is quite simple:

```c
if (condition) {
  // code if condition is true
  print "Condition is true";
} 
```


If the code that comes after the if spans only one line then the following is also valid:


```c
if (condition)
  print "Condition is true";
```

Else statements are also relatively simple and can just be added to the above like so:


```c
if (condition) {
  // code if condition is true
  print "Condition is true";
} else {
  print "Condition is false";
}
```

Again if it only spans one line:


```c
if (condition)
  print "Condition is true";
else
  print "Condition is false";
```

There is no `elif` keyword you just chain if and else together.

```c
if (condition) {
  // code if condition is true
  print "Condition is true";
} else if (otherCondition) {
  print "Other condition";
} else {
  print "Condition is false";
}
```

And again:


```c
if (condition)
  print "Condition is true";
else if (otherCondition)
  print "otherCondition is true";
else
  print "Condition is false";
```

Switch
------

Switch case statements use the C style syntax. If none of the cases are true it runs the code under the default vaue. Switch cases are usually faster than If/Else statemts so use them if you can.
```c
switch (age) {
  ...
  case 16:
    print "not old enough to drive or vote";
  case 17:
    print "can drive";
  case 18:
    print "can vote";
  default:
    print "can drive and vote";
}
```

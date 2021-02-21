# Example Code

Here are some sample programs

## Simple Examples

### 1. Hello World
Hello world can be done in two ways, imperatively or as a function (best practice).
```
// imperative
print "Hello, World!";
```
```
fn main() 
{
    print "Hello, World!";
}

main();
```
### 2. Read a file
Reading a file can be done with the builtin read() function
```
var src = read("main.mt");

print src;
```

### 3. Writing to a file
Writing to a file is similar to reading a file
```
var ok = write("hello.txt", "Hello, World!");

if (ok == 1) 
{
    print "Couldn't read file";
}
```

### 4. Working with strings
You can add two strings together with the plus operator
```
var str1 = "Hello, ";
var str2 = str1 + "World!";
print str2;
```
### 5. Working with time
You can use the system clock to profile the execution of functions
```
var start = clock();
someFunction();
var elapsed = clock() - start;
print elapsed;
```
### 6. Boolean Values
Boolean values are as you would expect
```
var b1 = true;
var b2 = false;
print !(b1 && b2 || !b1)
```
### 7. if/else 

The preferred mt style is to put the braces on a new line
```
var temp = 30;

if (temp < 16) 
{
    print "chilly";
} 
else if (temp < 28) 
{
    print "perfect";
}
else
{
    print "too hot";
}
```

### 8. while loops
While loops have c-style syntax
```
while (x < 10) 
{
    print x;
    x = x + 1;
}
```
### 9. for loops
For loops also use c-style syntax
```
for (var i = 0; i < 10; i = i + 1) 
{
    print i;
}
```

### 10. functions
Functions are most similar to python in their declaration
```
fn fib(n) 
{
    if (n <= 1) 
    {
        return n;
    }
    return fib(n-1) + fib(n-2);
}
```

### 11. Classes and Objects

Classes are defined slightly differently to most languages
```

class Person 
{
  init(name, age) 
  {
    this.name = name;
    this.age = age;
  }

  speak() 
  {
    print "Hello, I am " + this.name;
  }

  sayage() 
  {
    print "I am " + string(age) + " years old";
  }
}

var bob = Person("Bob", 20);
bob.speak();
bob.sayage();
bob.age = 21;
bob.sayage();

```

### 12. Importing Code
Lets say you have a file called `hello.mt` with the following code in it:
```
fn sayHello(name) 
{
  print "Hello " + name;
}
```
A second file `main.mt` can use the code in `hello.mt` as such
```
use "hello.mt";

sayHello("Bob");
```

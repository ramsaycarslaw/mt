# The mt Programming Language

> C meets Python... one day

## Install

### macOS
Make sure you have the XCode command line tools installed
```
git clone https://github.com/ramsaycarslaw/mt.git
cd mt
make
```

### GNU/Linux
equires a small modification to the Makefile as well as having gcc and make installed. In the Makefile: change 
```
CC = clang
```
to gcc or whatever compiler you use.
```
CC = gcc
```
And then finally:
```
git clone https://github.com/ramsaycarslaw/mt.git
cd mt
make
```

## Usage

You can call mt in REPL mode like:
```
./mt
```

Or pass in a file with:
```
./mt path/to/file
```

Alternativley you can copy the mt executable to /usr/local/bin/ to make it avaliable system wide.

## Simple Examples

### 1. Hello World
Hello world can be done in two ways, impertivley or as a function (best practice).
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
You can add stwo strings together with the plus operator
```
var str1 = "Hello, ";
var str2 = str1 + "World!";
print str2;
```
### 5. Working with time
You can use the system clock to profile the exectution of functions
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
The prefered mt style is to put the braces on a new line
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

### 10. functions
Functions are most similar to python in thier declaration
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

### Reference

| Operator/Keyword | Effect                                      |
| :----------------| -------------------------------------------:|
| +                | Adds two numbers or concatanates two strings|
| -                | subtracts two numbers or negates one        |
| *                | Multiplies two numbers                      |
| /                | Divides two numbers                         |
| ^                | raises the 1st num to the power of the  2nd |
| !                | Not, !true = false etc                      |
| ==               | is equal to                                 |
| !=               | Not equal to                                |
| <=               | less than or equal to                       |
| >=               | greater than equal to                       |
| <                | less than                                   |
| >                | greater than                                |




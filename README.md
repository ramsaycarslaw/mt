# The mt Programming Language

> C meets Python... one day

## Install

### macOS
Make sure you have the XCode command line tools installed
```
git clone https://github.com/ramsaycarslaw/mt.git
cd mt
make
make install
```

### GNU/Linux
Linux requires a small modification to the Makefile as well as having gcc and make installed. In the Makefile: change 
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
make install
```

## Uninstall

To uninstall mt, navigate to the directory with the makefile and run:
```
make uninstall
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

Alternatively, you can copy the mt executable to /usr/local/bin/ to make it available system wide.

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

***NOTE*** --- There is NO protection from infinite import loops - be careful! Also make sure the files are in the same folder, if they are not put the absolute path to the file in the string


## Advanced Examples

### FizzBuzz
The fizzBuzz problem is a classic interview question. In mt it can be solved like so:
```
// fizzBuzz where n is the max number
fn fizzBuzz(n) 
{
   var buffer = " ";
   
   for (var i = 0; i <= n; i = i + 1) 
   {
        if (i%3 == 0) 
        {
            buffer = buffer + "fizz";
        }
        
        if (i%5 == 0) 
        {
            buffer = buffer + "Buzz";
        }
        
        print string(i) + buffer;
        buffer = " ";
   }
}
```

### Reading from files
You can read and write to files using the built in read function.
```
fn main() 
{
    var str = read("path to file");
    
    str = str + "Some more text";
    
    write("new file.txt", str);
}
```

### Inheritance and the `super` method

Objects can inherit methods and values from other classes using the inheritance opera
. This is achieved using the `<` operator in a class name. The keyword `super` is used as a shorthand for inheritance.

```

class Shape {
  init(area) {
    this.area = area;
  }
}

class Circle < Shape {
  init(area) {
    super.init(area);
  }
  
  getRadius() {
    return (this.area / 3.1415)^(1/2);
  }
}

var c = Circle(100);
print c.getRadius();

```

### Reference

#### Operators

| Operator          | Effect                                       |
| :---------------- | :------------------------------------------- |
| +                 | Adds two numbers or concatanates two strings |
| -                 | subtracts two numbers or negates one         |
| *                 | Multiplies two numbers                       |
| /                 | Divides two numbers                          |
| ^                 | raises the 1st num to the power of the  2nd  |
| !                 | Not, !true = false etc                       |
| ==                | is equal to                                  |
| !=                | Not equal to                                 |
| <=                | less than or equal to                        |
| >=                | greater than equal to                        |
| <                 | less than                                    |
| >                 | greater than                                 |
| &&                | boolean and                                  |
| ||               | boolean or

#### Keywords

| Keyword                            | Effect                                      |
| :--------------------------------- | :------------------------------------------ |
| if (condition)                     | standard if statemnt                        |
| else                               | standard else, can be combined: else if     |
| while (condition)                  | while condition is true while loops         |
| for (declare; condition; statemnt  | c style for loops                           |
| var                                | declare a new variable to nil               |
| print                              | prints the following statement              |
| fn *name*(*args*)                  | declare a function                          |

#### Built In functions
| Function                          | Effect                                       |
| :-------------------------------- | :------------------------------------------- |
| clock() -> *number*               | unix time in seconds                         |
| read(*path*) -> *sting*           | reads the file at the specified path         |
| write(*path*, *text*) -> *number* | writes text to file at path returns 0 if ok  |
| number(*value*) -> *number*       | changes any type to a number                 |
| string(*value*) -> *string*       | changes numbers and bools to string literals |
| raw()                             | Raw puts the shell in raw mode               |
| sleep(n)                          | Sleep for n seconds                          |
| println(s) & printf(s)            | works like C's printf                        |

## ToDo
> In no particular order
- [ ] Garbage Collector
- [ ] Optimiser
- [x] String Subscript Get
- [ ] String Subscript Set
- [x] Text editor (see https://github.com/ramsaycarslaw/charm)
- [x] Classes
- [ ] Standard libary
    - [ ] std linear algebra
    - [ ] std string manip
    - [ ] std network
    - [ ] std I/O










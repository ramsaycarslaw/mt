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
mt
```

Or pass in a file with:
```
mt path/to/file
```

Alternatively, you can copy the mt executable to /usr/local/bin/ to make it available system wide.


## Examples

See the [docs](https://github.com/ramsaycarslaw/mt/docs/example.md)


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
| ?:                | ternary operator                             |
| \|\|                   | Boolean or                                   |

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
| use "path/to/file";                | import code from the path specified         |
| switch (condition)                 | switch case statement                       |
| case *expression*:                 | switch case option                          |
| default:                           | default value of switch case                |
| break                              | leave a loop early                          |
| continue                           | skip  to next loop cycle                    |

#### Built In functions
| Function                          | Effect                                       |
| :-------------------------------- | :------------------------------------------- |
| clock() -> *number*               | unix time in seconds                         |
| read(*path*) -> *sting*           | reads the file at the specified path         |
| write(*path*, *text*) -> *number* | writes text to file at path returns 0 if ok  |
| number(*value*) -> *number*       | changes any type to a number                 |
| string(*value*) -> *string*       | changes numbers and bools to string literals |
| sleep(n)                          | Sleep for n seconds                          |
| println(s) & printf(s)            | works like C's printf                        |

#### Modules

##### http:

The `http` module is how mt will commincate with websites and servers.

| Function                 | Effect                                               |
| -----------              | -----------                                          |
| http.Get(hostname, port) | Get the html file from hostname, optionally set port |

##### assert

The assert module is used for type checking and safety, all of the functions will create a runtime error if their conditions are not met. You can use them to be sure of the values you are recieving. 

| Function               | Effect                     |
| ---------------------  | ------------------         |
| assert.True(vals...)   | check some values are true |
| assert.False(vals...)  | check values are false     |
| assert.Equals(vals...) | check values are equal     |


Planned features
-----------------

The order of theses could change

1. For in loops

```
for x in y {
  print y;
}
```

2. range operator
```
for x in 0..10 {
  print x;  
}
```

3. Asynchronous Function Exectution (corountines)

```
async add(x, y) { return x+y; }
```

4. Optional Static Typing

```
let x: number = 10;
```


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
    - [ ] std network (in progress)
    - [ ] std I/O
    - [ ] std assert (in progress)










# mt

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

## mt supported commands

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

## Examples

### 1. Ackerman's function
```
  1 fn ack(m, n)                                                                    
  2 {                                                                               
  3     if (m == 0)                                                                 
  4     {                                                                           
  5         return n+1;                                                             
  6     }                                                                           
  7     else if (n == 0)                                                            
  8     {                                                                           
  9         return ack(m-1, 1);                                                     
 10     }                                                                           
 11                                                                                 
 12     return ack(m-1, ack(m, n - 1));                                             
 13 }                                                                               
 14                                                                                 
 15 print ack(3, 1);                                                                
                     
```

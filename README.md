# mt

> C meets Python... one day

## Install

### macOS
Make sure you have the XCode command line tools installed
'''
git clone https://github.com/ramsaycarslaw/mt.git
cd mt
make
'''

### GNU/Linux
Requires a small modification to the Makefile as well as having gcc and make installed. In the Makefile: change 
'''
CC = clang
'''
to gcc or whatever compiler you use.
'''
CC = gcc
'''
And then finally:
'''
git clone https://github.com/ramsaycarslaw/mt.git
cd mt
make
'''

## Usage

You can call mt in REPL mode like:
'''
./mt
'''

Or pass in a file with:
'''
./mt path/to/file
'''

## mt supported commands
Currently, only *,+,/,- and brackets are supported, example:
'''
(1 + 2 - (10 / 3))
'''


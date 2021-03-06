CC = clang

CFLAGS = -c -std=c99 -g -Wall -O3 -funsafe-math-optimizations
LDFLAGS = -g
SRC = $(wildcard ./src/*.c)
SRC += $(wildcard ./module/*.c)
HDR = $(wildcard ./include/*.h)
HDR += $(wildcard ./module/*.h)
OBJ = $(SRC:.c=.o)
EXEC = mt
PREFIX = /usr/local

all: $(SRC) $(OBJ) $(EXEC)

$(EXEC): $(OBJ)
	$(CC) $(LDFLAGS) $^ -o $@ -lm

%.o: %.c $(HDR)
	$(CC) $(CFLAGS) $< -o $@ 

.PHONY: install
install: $(EXEC)
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp $< $(DESTDIR)$(PREFIX)/bin/mt

.PHONY: uninstall
uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/mt

clean:
	rm ./src/*.o $(EXEC)



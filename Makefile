CC = clang
CFLAGS = -c -std=c99 -g -Wall -O3 -funsafe-math-optimizations
LDFLAGS = -g
SRC = $(wildcard ./src/*.c)
HDR = $(wildcard ./include/*.h)
OBJ = $(SRC:.c=.o)
EXEC = ./bin/mt
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

.PHONY: check
check:
	./test/test

clean:
	rm ./src/*.o $(EXEC)



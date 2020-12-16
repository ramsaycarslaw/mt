CC = clang
CFLAGS = -c -std=c11 -g -Wall -O3 -funsafe-math-optimizations -lm
LDFLAGS = -g
SRC = $(wildcard ./*.c)
HDR = $(wildcard ./*.h)
OBJ = $(SRC:.c=.o)
EXEC = mt
PREFIX = /usr/local

all: $(SRC) $(OBJ) $(EXEC)

$(EXEC): $(OBJ)
	$(CC) $(LDFLAGS) $^ -o $@

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
	rm *.o $(EXEC)



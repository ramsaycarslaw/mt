CC = clang
CFLAGS = -c -std=c99 -g -Wall
LDFLAGS = -g
SRC = $(wildcard ./*.c)
HDR = $(wildcard ./*.h)
OBJ = $(SRC:.c=.o)
EXEC = mt

all: $(SRC) $(OBJ) $(EXEC)

$(EXEC): $(OBJ)
	$(CC) $(LDFLAGS) $^ -o $@

%.o: %.c $(HDR)
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm src/*.o $(EXEC)



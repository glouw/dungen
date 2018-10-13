BIN = dungen

CFLAGS = -std=c99 -Wall -Wextra -pedantic -Ofast -flto -march=native

LDFLAGS = -lm

CC = gcc

SRC = main.c Map.c

all:
	$(CC) $(CFLAGS) $(LDFLAGS) $(SRC) -o $(BIN)

run:
	./$(BIN)

clean:
	rm -f $(BIN)

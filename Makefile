CC = mpicc
CFLAGS = -Wall -Werror -Wextra -pedantic
LDFLAGS =
EXEC = projet
SRC = test.c $(EXEC).c
OBJ = $(SRC: .c = .o)

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

.PHONY: clean mrproper

clean:
	rm -rf *.o

mrproper: clean
	rm -rf $(EXEC)

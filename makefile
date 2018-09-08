.PHONY: clean # This creates the label which can be called

CC = gcc 
CFLAGS = -Wall -pedantic
DEPS = 
OBJ = assembler.o

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

assembler: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^


clean:
	rm $(OBJ)
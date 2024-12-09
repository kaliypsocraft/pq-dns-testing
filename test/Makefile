# Compiler and flags
CC = gcc
CFLAGS = -Wall -g 

# List of object files (common to both executables)
COMMON_OBJ = dns_message.o packedrr.o question.o resource_record.o rrfrag.o handler.o

# Object files specific to each executable
SANDBOX_OBJ = sandbox.o
ORIGINAL_OBJ = original.o

# Targets to create the executables
all: arrf_dynamic arrf_static

arrf_dynamic: $(COMMON_OBJ) $(SANDBOX_OBJ)
	$(CC) $(CFLAGS) -o arrf_dynamic $(COMMON_OBJ) $(SANDBOX_OBJ) -lm

arrf_static: $(COMMON_OBJ) $(ORIGINAL_OBJ)
	$(CC) $(CFLAGS) -o arrf_static $(COMMON_OBJ) $(ORIGINAL_OBJ) -lm

# Rules for common object files
dns_message.o: dns_message.c
	$(CC) $(CFLAGS) -c dns_message.c

packedrr.o: packedrr.c
	$(CC) $(CFLAGS) -c packedrr.c

question.o: question.c
	$(CC) $(CFLAGS) -c question.c

resource_record.o: resource_record.c
	$(CC) $(CFLAGS) -c resource_record.c

rrfrag.o: rrfrag.c
	$(CC) $(CFLAGS) -c rrfrag.c

handler.o: handler.c
	$(CC) $(CFLAGS) -c handler.c

# Rules for specific object files
sandbox.o: sandbox.c
	$(CC) $(CFLAGS) -c sandbox.c

original.o: original.c
	$(CC) $(CFLAGS) -c original.c

# Clean up object files and executables
clean:
	rm -f *.o arrf_dynamic arrf_static

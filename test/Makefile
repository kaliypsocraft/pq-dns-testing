# Compiler and flags
CC = gcc
CFLAGS = -Wall -g 

# List of object files
OBJ = dns_message.o packedrr.o question.o resource_record.o rrfrag.o sandbox.o -lm

# Target to create the executable
server: $(OBJ)
	$(CC) $(CFLAGS) -o server $(OBJ)

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

sandbox.o: sandbox.c
	$(CC) $(CFLAGS) -c sandbox.c

server.o: server.c
	$(CC) $(CFLAGS) -c server.c

# Clean up object files and executable
clean:
	rm -f *.o server

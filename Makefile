all : jinv

# compiler
CC = gcc

# options while development
CFLAGS = -ggdb -Wall 

# options for release
# CFLAGS = -O -Wall

jinv : main.o set_environment.o jinv_environment.o jinv_allocator.o jinv_command.o
	$(CC) $(CFLAGS) -o jinv main.o set_environment.o jinv_environment.o jinv_allocator.o jinv_command.o -lncurses
main.o : main.c jinv.h
	$(CC) $(CFLAGS) -c main.c
set_environment.o : set_environment.c jinv.h
	$(CC) $(CFLAGS) -c -lncurses set_environment.c
jinv_environment.o : jinv_environment.c jinv.h
	$(CC) $(CFLAGS) -c -lncurses jinv_environment.c
jinv_allocator.o : jinv_allocator.c jinv.h
	$(CC) $(CFLAGS) -c jinv_allocator.c
jinv_command.o : jinv_command.c jinv.h
	$(CC) $(CFLAGS) -c jinv_command.c

clean :
	-rm main.o set_environment.o jinv_environment.o jinv_allocator.o jinv_command.o

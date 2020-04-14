CFLAGS = -g -Werror -pthread -std=c99 -o test
all: test.o
test.o: test.c concurrent_list.c concurrent_list.h
	gcc $(CFLAGS) test.c concurrent_list.c

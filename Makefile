CC=gcc
CFLAGS=-m32 -std=c99


main:
	$(CC) $(CFLAGS) -o schedulers src/schedulers.c

clean:
	rm schedulers 
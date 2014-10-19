CC=gcc
CFLAGS=-m32 -std=c99


main:
	$(CC) $(CFLAGS) -o schedulers schedulers.c

clean:
	rm main 
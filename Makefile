CC=gcc
CFLAGS=-m32 -std=c99


main:
	$(CC) $(CFLAGS) -o main schedulers.c

clean:
	rm main 
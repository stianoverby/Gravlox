CC=gcc
EXTRA_CFLAGS=-fsanitize=address -fsanitize=undefined -fsanitize=leak -fsanitize=bounds -fanalyzer -g
CFLAGS=-std=c99 -Wall -Wextra -pedantic
OBJFILES=test.o lutils.o interpreter.o
HEADERFILES=interpreter.h lutils.h
EXE=test

ifeq ($(USE_EXTRA_CFLAGS),1)
	CFLAGS += $(EXTRA_CFLAGS)
endif

.PHONY: clean

$(EXE): test.o lutils.o interpreter.c
	$(CC) $(CFLAGS) -o test test.o lutils.o

test.o: test.c lutils.h interpreter.h
	$(CC) $(CFLAGS) -c test.c

lutils.o: lutils.c lutils.h
	$(CC) $(CFLAGS) -c lutils.c

interpreter.o: interpreter.c lutils.h interpreter.h
	$(CC) $(CFLAGS) -c interpreter.c

clean:
	rm -f $(EXE) *.o

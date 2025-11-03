CC=gcc
EXTRA_CFLAGS=-fsanitize=address -fsanitize=undefined -fsanitize=leak -fsanitize=bounds -fanalyzer -g
CFLAGS=-std=c99 -Wall -Wextra -pedantic
OBJFILES=main.o lutils.o interpreter.o
HEADERFILES=interpreter.h lutils.h
EXE=main

ifeq ($(USE_EXTRA_CFLAGS),1)
	CFLAGS += $(EXTRA_CFLAGS)
endif

.PHONY: clean

$(EXE): $(OBJFILES)
	$(CC) $(CFLAGS) -o main $(OBJFILES)

main.o: main.c lutils.h interpreter.h
	$(CC) $(CFLAGS) -c main.c

lutils.o: lutils.c lutils.h
	$(CC) $(CFLAGS) -c lutils.c

interpreter.o: interpreter.c lutils.h interpreter.h
	$(CC) $(CFLAGS) -c interpreter.c

clean:
	rm -f $(EXE) *.o

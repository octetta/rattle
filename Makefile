all: bin lib rattle

rattle: lib/libamy.a lib/librma.a src/main.c src/rattlefy.c
	$(CC) -Isrc -Iamy/src src/main.c src/rattlefy.c -o bin/rattle -Llib -lrma -lamy -lm

lib/librma.a: lib
	gcc -Iamy/src -c src/rma.c -o lib/rma.o
	ar -cvq lib/librma.a lib/rma.o
	ranlib lib/librma.a

lib/libamy.a: lib
	gcc -c amy/src/algorithms.c   -o lib/algorithms.o
	gcc -c amy/src/amy.c          -o lib/amy.o
	gcc -c amy/src/delay.c        -o lib/delay.o
	gcc -c amy/src/envelope.c     -o lib/envelope.o
	gcc -c amy/src/filters.c      -o lib/filters.o
	gcc -c amy/src/log2_exp2.c    -o lib/log2_exp2.o
	gcc -c amy/src/oscillators.c  -o lib/oscillators.o
	gcc -c amy/src/partials.c     -o lib/partials.o
	gcc -c amy/src/pcm.c          -o lib/pcm.o
	ar -cvq lib/libamy.a \
    lib/algorithms.o \
    lib/amy.o \
    lib/delay.o \
    lib/envelope.o \
    lib/filters.o \
    lib/log2_exp2.o \
    lib/oscillators.o \
    lib/partials.o \
    lib/pcm.o \
    #
	ranlib lib/libamy.a

bin:
	mkdir bin

lib:
	mkdir lib

clean:
	rm -f bin/*
	rmdir bin
	rm -f lib/*
	rmdir lib

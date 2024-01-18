all: bin lib bin/rattle

bin/rattle: lib/libamy.a lib/librma.a src/main.c src/rattlefy.c
	$(CC) \
    -Isrc \
    -Iamy/src \
    -Ilinenoise \
    -Itinycthread/source \
    src/main.c \
    src/rattlefy.c \
    linenoise/linenoise.c \
    tinycthread/source/tinycthread.c \
    -o bin/rattle \
    -Llib \
    -lrma \
    -lamy \
    -lm \
    #

lib/librma.a: \
    src/rma.c \
    src/rma.h \
    #
	mkdir -p lib
	gcc -Iamy/src -c src/rma.c -o lib/rma.o
	ar -cvq lib/librma.a lib/rma.o
	ranlib lib/librma.a

lib/libamy.a: \
	amy/src/algorithms.c \
	amy/src/amy.c \
	amy/src/delay.c \
	amy/src/envelope.c \
	amy/src/filters.c \
	amy/src/log2_exp2.c \
	amy/src/oscillators.c \
	amy/src/partials.c \
	amy/src/pcm.c \
  #
	mkdir -p lib
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

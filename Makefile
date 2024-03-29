all: bin lib bin/rattle bin/rmini bin/timer02 bin/grattle

ifeq ($(shell uname), Linux)
LL = -lrt -ldl -latomic
else
CC = clang
LL = -framework AudioUnit -framework CoreAudio -framework CoreFoundation 
endif

bin/timer02: src/timer02.go
	go build -o $@ $<

bin/grattle: src/grattle.go lib/libamy.a lib/librma.a lib/librat.a
	go mod tidy
	go build -o $@ $<

bin/rmini: lib/libamy.a lib/librma.a lib/librat.a src/rmini.c
	$(CC) \
    -Isrc \
    -Iamy/src \
    src/rmini.c \
    -Llib \
    -lrat \
    -lrma \
    -lamy \
    -lm \
    -lpthread \
    $(LL) \
    -o bin/rmini \
    #

bin/rattle: lib/libamy.a lib/librma.a src/main.c src/rattlefy.c
	$(CC) \
    -Isrc \
    -Iamy/src \
    -Ilinenoise \
    src/main.c \
    src/rattlefy.c \
    linenoise/linenoise.c \
    -Llib \
    -lrma \
    -lamy \
    -lm \
    -lpthread \
    $(LL) \
    -o bin/rattle \
    #

lib/librma.a: \
    src/rma.c \
    src/rma.h \
    #
	mkdir -p lib ; rm -f lib/librma.a
	gcc -Iamy/src -c src/rma.c -o lib/rma.o
	ar -cvq lib/librma.a lib/rma.o
	ranlib lib/librma.a

lib/librat.a: src/ratlib.c src/ratlib.h \
    src/ratlib.c \
    src/ratlib.h \
    #
	mkdir -p lib ; rm -f lib/librat.a
	gcc -Iamy/src -c src/ratlib.c -o lib/ratlib.o
	ar -cvq lib/librat.a lib/ratlib.o
	ranlib lib/librat.a

lib/libamy.a: \
  src/my_logging.c \
	amy/src/algorithms.c \
	amy/src/amy.c \
	amy/src/delay.c \
	amy/src/envelope.c \
	amy/src/examples.c \
	amy/src/filters.c \
	amy/src/log2_exp2.c \
	amy/src/oscillators.c \
	amy/src/partials.c \
	amy/src/pcm.c \
  #
	mkdir -p lib ; rm -f lib/libamy.a
	gcc -c src/my_logging.c       -o lib/my_logging.o
	gcc -c amy/src/algorithms.c   -o lib/algorithms.o
	gcc -I src -c amy/src/amy.c   -o lib/amy.o
	gcc -c amy/src/delay.c        -o lib/delay.o
	gcc -c amy/src/envelope.c     -o lib/envelope.o
	gcc -c amy/src/examples.c     -o lib/examples.o
	gcc -c amy/src/filters.c      -o lib/filters.o
	gcc -c amy/src/log2_exp2.c    -o lib/log2_exp2.o
	gcc -c amy/src/oscillators.c  -o lib/oscillators.o
	gcc -c amy/src/patches.c      -o lib/patches.o
	gcc -c amy/src/partials.c     -o lib/partials.o
	gcc -c amy/src/pcm.c          -o lib/pcm.o
	ar -cvq lib/libamy.a \
    lib/my_logging.o \
    lib/algorithms.o \
    lib/amy.o \
    lib/delay.o \
    lib/envelope.o \
    lib/examples.o \
    lib/filters.o \
    lib/log2_exp2.o \
    lib/oscillators.o \
    lib/patches.o \
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

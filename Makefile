all: bin lib bin/rattle bin/rmini bin/timer02 bin/grattle

ifeq ($(shell uname), Linux)
LL = -lrt -ldl -latomic
else
CC = clang
LL = -framework AudioUnit -framework CoreAudio -framework CoreFoundation 
endif

# AMY = amy
AMY = amy-alt

bin/timer02: src/timer02.go
	go build -o $@ $<

bin/grattle: src/grattle.go lib/libamy.a lib/librma.a lib/librat.a
	go mod tidy
	go build -o $@ $<

bin/rmini: lib/libamy.a lib/librma.a lib/librat.a src/rmini.c
	$(CC) \
    -DPCM_MUTABLE \
    -Isrc \
    -I$(AMY)/src \
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
    -DPCM_MUTABLE \
    -Isrc \
    -I$(AMY)/src \
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
	gcc -I$(AMY)/src -c src/rma.c -o lib/rma.o
	ar -cvq lib/librma.a lib/rma.o
	ranlib lib/librma.a

lib/librat.a: src/ratlib.c src/ratlib.h \
    src/ratlib.c \
    src/ratlib.h \
    #
	mkdir -p lib ; rm -f lib/librat.a
	gcc -DPCM_MUTABLE -I$(AMY)/src -c src/ratlib.c -o lib/ratlib.o
	ar -cvq lib/librat.a lib/ratlib.o
	ranlib lib/librat.a

lib/libamy.a: \
  src/my_logging.c \
	$(AMY)/src/algorithms.c \
	$(AMY)/src/amy.c \
	$(AMY)/src/custom.c \
	$(AMY)/src/delay.c \
	$(AMY)/src/envelope.c \
	$(AMY)/src/examples.c \
	$(AMY)/src/filters.c \
	$(AMY)/src/log2_exp2.c \
	$(AMY)/src/oscillators.c \
	$(AMY)/src/partials.c \
	$(AMY)/src/pcm.c \
  #
	mkdir -p lib ; rm -f lib/libamy.a
	gcc -c src/my_logging.c       -o lib/my_logging.o
	gcc -DPCM_MUTABLE -c $(AMY)/src/algorithms.c   -o lib/algorithms.o
	gcc -DPCM_MUTABLE -I src -c $(AMY)/src/amy.c   -o lib/amy.o
	gcc -DPCM_MUTABLE -c $(AMY)/src/custom.c       -o lib/custom.o
	gcc -DPCM_MUTABLE -c $(AMY)/src/delay.c        -o lib/delay.o
	gcc -DPCM_MUTABLE -c $(AMY)/src/envelope.c     -o lib/envelope.o
	gcc -DPCM_MUTABLE -c $(AMY)/src/examples.c     -o lib/examples.o
	gcc -DPCM_MUTABLE -c $(AMY)/src/filters.c      -o lib/filters.o
	gcc -DPCM_MUTABLE -c $(AMY)/src/log2_exp2.c    -o lib/log2_exp2.o
	gcc -DPCM_MUTABLE -c $(AMY)/src/oscillators.c  -o lib/oscillators.o
	gcc -DPCM_MUTABLE -c $(AMY)/src/patches.c      -o lib/patches.o
	gcc -DPCM_MUTABLE -c $(AMY)/src/partials.c     -o lib/partials.o
	gcc -DPCM_MUTABLE -c $(AMY)/src/pcm.c          -o lib/pcm.o
	ar -cvq lib/libamy.a \
    lib/my_logging.o \
    lib/algorithms.o \
    lib/amy.o \
    lib/custom.o \
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

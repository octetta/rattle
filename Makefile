all: bin lib rattle

rattle: lib/libamy.a src/main.c
	$(CC) -I amy/src src/main.c -o bin/rattle -L lib -lamy -lm

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
	gcc -I miniaudio -c amy/src/libminiaudio-audio.c -o lib/libminiaudio-audio.o
	ar -cvq lib/libamy.a lib/*.o
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

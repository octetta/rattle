rattle: bin lib src/main.c
	$(CC) src/main.c -o bin/rattle

bin:
	mkdir bin

lib:
	mkdir lib

clean:
	rm -f bin/*
	rmdir bin
	rm -f lib/*
	rmdir lib

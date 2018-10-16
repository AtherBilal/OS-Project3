CC=gcc
CFLAGS=-Wall -std=gnu99
EXECUTABLES=oss child
HELPERFILES= src/validation.c
all: $(EXECUTABLES)

oss: src/oss.c
	mkdir -p bin/
	touch bin/CreateKeyFile
	touch bin/log
	$(CC) $(CFLAGS) $< $(HELPERFILES) -o bin/$@
child:src/child.c
	$(CC) $(CFLAGS) $< $(HELPERFILES) -o bin/$@
clean:
	rm -rf bin/

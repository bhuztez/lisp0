SRCS=$(wildcard src/*.c) $(wildcard src/*.s)
HEADERS=$(wildcard src/*.h)
CFLAGS=-Os -std=gnu99 -Wall -Wextra -Werror
LDFLAGS=-s -flto -static -nostartfiles -nostdlib -nodefaultlibs -nostdlib

lisp0.elf: $(SRCS) $(HEADERS)
	gcc $(CFLAGS) $(LDFLAGS) -o "$@" $(SRCS)

clean:
	rm -rf *.elf

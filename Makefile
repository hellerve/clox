TARGET=clox
BUILDDIR=bin/
PREFIX=/usr/local/bin/
SOURCES=$(wildcard src/*.c)
MAIN=main.c
override CFLAGS+=-Werror -Wall -g -fPIC -O2 -DNDEBUG -ftrapv -Wundef -Wwrite-strings -Wuninitialized -pedantic -std=c11 -fsanitize=address
override LDFLAGS+=-lreadline

all: main.c
	mkdir -p $(BUILDDIR)
	$(CC) $(MAIN) $(SOURCES) -o $(BUILDDIR)$(TARGET) $(CFLAGS) $(LDFLAGS)

install: all
	install $(BUILDDIR)$(TARGET) $(PREFIX)$(TARGET)

uninstall:
	rm -rf $(PREFIX)$(TARGET)

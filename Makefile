TARGET = httpserver
LIBS = -lmicrohttpd -lz -ljansson
INCLUDES = $(HOME)/Library/libmicrohttpd/include/
LIBRARIES = $(HOME)/Library/libmicrohttpd/lib/
SOURCES = routes.c  auth.c httpserver.c
CC = gcc
CFLAGS = -g -Wall

default: build
build:
	$(CC) $(CFLAGS) -I$(INCLUDES) -L$(LIBRARIES) $(SOURCES) $(LIBS) -o $(TARGET)

clean :
	rm httpserver
	rm -R *.dSYM

run:
	./httpserver


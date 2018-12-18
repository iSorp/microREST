TARGET = httpserver
LIBS = -lmicrohttpd -lz -ljansson
INCLUDES = $(HOME)/Library/libmicrohttpd/include/
LIBRARIES = $(HOME)/Library/libmicrohttpd/lib/
SOURCES = routes.c urlparser.c httpserver.c auth.c
CC = gcc
CFLAGS = -g -Wall

default: build
build:
	$(CC) $(CFLAGS) -I$(INCLUDES) -L$(LIBRARIES) $(SOURCES) $(LIBS) -o $(TARGET)

clean :
	rm httpserver

run:
	./httpserver


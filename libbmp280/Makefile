# options
STANDALONE=no

LIB_PATH = $(HOME)
#/Library/libmicrohttpd

SRC_DIR  	= src
TARGET 		= libbmp280.a
INCLUDES 	= -I$(wildcard $(SRC_DIR))
LIBRARIES	= -L$(LIB_PATH)/lib/
OBJ_FILES 	= $(patsubst %.c, %.o, $(wildcard $(SRC_DIR)/*.c))

CFLAGS 		= -c -Wall
LDFLAGS		= 
CC      	= gcc 
LD      	= gcc

# Compile with i2c
ifeq ($(STANDALONE),yes)
	CPPFLAGS += -DSTANDALONE
endif


all: $(TARGET)

$(TARGET) : $(OBJ_FILES)
	ar rcs -o $@ $(OBJ_FILES)

$(OBJ_FILES): %.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(INCLUDES) $< -o $@

install:
	cp libbmp280.a $(LIB_PATH)/lib
	cp $(SRC_DIR)/*.h $(LIB_PATH)/include

test:
	$(CC) -g -Wall -I$(LIB_PATH)/include -L$(LIB_PATH)/lib main.c -lbmp280 -o main.out
	./main.out

clean :
	rm -R *.out $(SRC_DIR)/*.o *.a *.dSYM






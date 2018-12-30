# options
SENSOR=no

LIB_PATH 	= $(HOME)/Library/libmicrohttpd
SRC_DIR  	= src
LIBS 	 	= -lmicrohttpd -lz -ljansson -lm
INCLUDES 	= -I$(LIB_PATH)/include/
LIBRARIES	= -L$(LIB_PATH)/lib/
OBJ_FILES 	= $(patsubst %.c, %.o, $(wildcard $(SRC_DIR)/*.c))
TARGET 		= httpserver.out

CFLAGS 		= -c -Wall
LDFLAGS		= -g
CC      	= gcc 
LD      	= gcc

ifeq ($(SENSOR),yes)
	LIBS += -lbmp280
else
	CPPFLAGS += -DNO_SENSOR
endif

export LD_LIBRARY_PATH=$(LIB_PATH)/lib


all: $(TARGET)

$(TARGET) : $(OBJ_FILES)
	$(LD) $(LDFLAGS) -o $@ $(OBJ_FILES) $(LIBRARIES) $(LIBS)

$(OBJ_FILES): %.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(INCLUDES) $< -o $@

clean :
	rm -R *.out $(SRC_DIR)/*.o $(SRC_DIR)/*.a *.dSYM

run:
	./httpserver.out
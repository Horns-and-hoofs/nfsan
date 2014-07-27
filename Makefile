CC=g++
INCLUDES=-I./include/ -I../include
#CCFLAGS=-g -Wall -fPIC

src=src/*.cc src/*.c

FRAMEWORK_LIB=-lpcap -lboost_thread-mt 
LIB_PATH = -L../lib/ -L.

OUT_BIN=prog1
all: framework_exe
	rm -f *.o
framework_exe:
	$(CC)  $(INCLUDES) $(CCFLAGS)  $(src)  $(LIB_PATH) $(FRAMEWORK_LIB) -o $(OUT_BIN)

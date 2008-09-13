OPT   = -O3 -march=native -fomit-frame-pointer -DDEBUG -ffast-math -frename-registers
DEBUG = -ggdb3 -DDEBUG -fno-inline 
PROF  = -O2 -march=native -DDEBUG -ggdb3 -fno-inline 

CFLAGS += -Wall 

GPP    = g++ $(CFLAGS) 

h_FILES   = board.h getMove.h engine.h utils.h
H_FILES   = board.cpp getMove.cpp engine.cpp utils.cpp
O_FILES   = board.o getMove.o engine.o utils.o

all: akimot_debug 

akimot_debug: $(O_FILES)
	$(GPP) $(DEBUG) -o akimot_debug getMove.o board.o engine.o utils.o

.SUFFIXES: .cpp .o

.cpp.o:
	$(GPP) -c $<

clean:
	rm -f *.o
	rm -f akimot_debug

run:
	./akimot_debug positions/startpos.txt

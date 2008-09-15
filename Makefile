OPT   = -O3 -march=native -fomit-frame-pointer -ffast-math -frename-registers
DEBUG = -ggdb3 -DDEBUG -fno-inline 
PROF  = -O2 -march=native -DDEBUG -ggdb3 -fno-inline 

CFLAGS += -Wall 

GPP    = g++ $(CFLAGS) 

H_FILES   = board.h engine.h utils.h
C_FILES   = board.cpp getMove.cpp engine.cpp utils.cpp
O_FILES   = board.o getMove.o engine.o utils.o

all: debug 

debug: $(O_FILES)
	$(GPP) $(DEBUG) -o akimot getMove.o board.o engine.o utils.o

opt: $(O_FILES)
	$(GPP) $(OPT) -o akimot getMove.o board.o engine.o utils.o

prof: $(O_FILES)
	$(GPP) $(PROF) -o akimot getMove.o board.o engine.o utils.o

board.o: board.cpp board.h utils.h
	$(GPP) -c board.cpp

engine.o: engine.cpp engine.h utils.h board.h
	$(GPP) -c engine.cpp

utils.o: utils.cpp utils.h
	$(GPP) -c utils.cpp

getMove.o: getMove.cpp
	$(GPP) -c getMove.cpp


#.SUFFIXES: .cpp .o
#.cpp.o: $(H_FILES)
#	$(GPP) -c $<

clean:
	rm -f *.o
	rm -f akimot

run:
	./akimot positions/startpos.txt

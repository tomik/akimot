OPT   = -O3 -march=native -fomit-frame-pointer -DDEBUG -ffast-math -frename-registers
DEBUG = -ggdb3 -DDEBUG -fno-inline 
PROF  = -O2 -march=native -DDEBUG -ggdb3 -fno-inline 

CFLAGS += -Wall 

GPP    = g++ $(CFLAGS) 

FILES  = Makefile board.cpp getMove.cpp search.cpp hash.cpp

all: akimot_debug

akimot_debug: $(FILES)
	$(GPP) $(DEBUG) -o akimot_debug main.cpp

akimot_opt:   $(FILES)
	$(GPP) $(OPT)   -o akimot_opt   main.cpp

akimot_asm:   $(FILES)
	$(GPP) $(OPT)   -S -c        main.cpp

akimot_prof:  $(FILES)
	$(GPP) $(PROF)  -o akimot_prof  main.cpp



.SUFFIXES: .cpp .o

.cpp.o:
	$(GPP) -c $<

clean:
	rm -f *.s *.o .depend gmon.out core
	rm -f akimot_debug akimot_opt akimot_prof
	rm -f *~
	rm -f *.orig

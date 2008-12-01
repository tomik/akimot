CC=gcc
# FLAGS= -O3 -Wall -mcpu=pentiumpro -fomit-frame-pointer
# FLAGS=-g -O0

#FLAGS= -g -O4 -Wall -mcpu=pentiumpro -fomit-frame-pointer
#FLAGS= -g -O4 -Wall
FLAGS= -g -O4 

LOPT=-static

all : getMove

getMove: getMove.c eval.c
	$(CC) $(LOPT) $(FLAGS) getMove.c -o getMove



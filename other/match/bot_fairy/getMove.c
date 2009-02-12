#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "board.h"
#include "search.h"
#include "hash.h"
#include "eval.h"

int main(int argc, char *argv[]) // returns 1 if an error occurs, 0 otherwise
{
    int error_code=0;
    board_t position;
    move_t moves[4];
    int i;
    int steps=0;
    
    BOARD_Message_Init();
    if (argc<2)
    {
        sprintf(message,"Program requires an argument (name of file containing position).\n");
        BOARD_Message();
        error_code=1;
    }
    {
        srand(0);
        HASH_Init();
        #ifdef TESTING
        {
            HASH_Set_Size(32*1024*1024);
        }
        #else
        {
            HASH_Set_Size(512*1024*1024);
        }
        #endif
        BOARD_Init(&position);
        if (BOARD_Read_Position(&position, argv[1]))
        {
            sprintf(message,"Couldn't read position from file.\n");
            BOARD_Message();
            error_code=1;
        } else
        {
            if (position.move==0)
            {
                printf("Ra1 Rb1 Rc1 Rd1 Re1 Rf1 Rg1 Rh1 Ha2 Db2 Cc2 Md2 Ee2 Cf2 Dg2 Hh2\n");
            } else if (position.move==1) 
            {
                if ((position.board[75] & PIECE_MASK)==ELEPHANT_PIECE)
                {
                    printf("ra8 rb8 rc8 rd8 re8 rf8 rg8 rh8 ha7 db7 cc7 ed7 me7 cf7 dg7 hh7\n");
                } else
                {
                    printf("ra8 rb8 rc8 rd8 re8 rf8 rg8 rh8 ha7 db7 cc7 md7 ee7 cf7 dg7 hh7\n");
                }
            } else
            {
                position.steps=0;
                BOARD_Calculate_Hashkey(&position);
                EVAL_Eval(&position,TRUE);
                SEARCH_Start_Search(&position,moves,NULL);
                for (i=0; steps<4; i++)
                {
                    BOARD_Send_Move(&moves[i]);
                    printf(" ");
                    steps+=moves[i].steps;
                }
                printf("\n");
            }
        }
    }
    BOARD_Message_Exit();
    return error_code;
}

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
    int i, j;
    int steps=0;
    test_data_t test_data;
    FILE *fp;
    char line[100];
    int number_of_tests=0;
    
    BOARD_Message_Init();
    if (argc<2)
    {
        sprintf(message,"Program requires an argument (name of file containing list of test positions).\n");
        BOARD_Message();
        error_code=1;
    }
    {
        srand(0);
        HASH_Init();
        BOARD_Init(&position);
        test_data.depth_limit=0;
        test_data.time=0;
        test_data.nodes=0;
        test_data.evals=0;
        fp=fopen(argv[1],"r");
        if (fp==NULL)
        {
            error_code=1;
        } else
        {
            fgets(line,100,fp);
            while (line[0]=='#')
            {
                fgets(line,100,fp);
            }
            for (i=0; line[i]>='0' && line[i]<='9'; i++)
            {
                number_of_tests=number_of_tests*10+line[i]-'0';
            }
            for (i=0; !error_code && i<number_of_tests; i++)
            {
                fgets(line,100,fp);
                while (line[0]=='#')
                {
                    fgets(line,100,fp);
                }
                for (j=0; line[j]!='\n'; j++)
                {
                }
                line[j]='\0';
                sprintf(message,"Reading position \"%s\" from file.\n",line);
                BOARD_Message();
                if (BOARD_Read_Position(&position,line))
                {
                    sprintf(message,"Couldn't read position from file.\n");
                    BOARD_Message();
                    error_code=1;
                } else
                {
                    fgets(line,100,fp);
                    while (line[0]=='#')
                    {
                        fgets(line,100,fp);
                    }
                    test_data.depth_limit=0;
                    for (j=0; line[j]>='0' && line[j]<='9'; j++)
                    {
                        test_data.depth_limit=test_data.depth_limit*10+line[j]-'0';
                    }
                    position.steps=0;
                    HASH_Set_Size(512*1024*1024);
                    EVAL_Eval(&position,TRUE);
                    SEARCH_Start_Search(&position,moves,&test_data);
                }
            }
            fclose(fp);
            sprintf(message,"Total time used: %.2f seconds\n",test_data.time);
            BOARD_Message();
            sprintf(message,"Total nodes searched: %d nodes\n",(unsigned long int)test_data.nodes);
            BOARD_Message();
            sprintf(message,"Total evals done: %d evals\n",(unsigned long int)test_data.evals);
            BOARD_Message();
        }
    }
    BOARD_Message_Exit();
    return 0;
}

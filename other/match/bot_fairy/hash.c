#include <stdlib.h>
#include <stdio.h>

#include "hash.h"

unsigned long long int hashkey_piece[100][OWNER_MASK|PIECE_MASK]; // hashkey values for each piece, on each square of the board.
unsigned long long int hashkey_step[4]; // hashkey values for how many steps have been.
unsigned long long int hashkey_at_move[OWNER_MASK]; // hashkey values for who is at move.

static hash_entry_t *hash_table=NULL;
static unsigned long long int hash_mask;

unsigned long long int Get_Random_Hashkey_Value(void)
{
    unsigned long long int hashkey_value=0;
    int i;
    unsigned long long int randbit;
    
    for (i=0; i<64; i++)
    {
        randbit=(unsigned long long int)rand()%2;
        hashkey_value=(hashkey_value<<1)+randbit;
    }
    return hashkey_value;
}

void HASH_Init(void)
{
    int i, j;
    
    for (i=0; i<100; i++)
    {
        for (j=0; j<(OWNER_MASK|PIECE_MASK); j++)
        {
            hashkey_piece[i][j]=0;
        }
    }
    for (i=0; i<4; i++)
    {
        hashkey_step[i]=0;
    }
    for (i=0; i<OWNER_MASK; i++)
    {
        hashkey_at_move[i]=0;
    }
    for (i=1; i<9; i++)
    {
        for (j=1; j<9; j++)
        {
            hashkey_piece[i*10+j][GOLD|ELEPHANT_PIECE]=Get_Random_Hashkey_Value();
            hashkey_piece[i*10+j][GOLD|CAMEL_PIECE]=Get_Random_Hashkey_Value();
            hashkey_piece[i*10+j][GOLD|HORSE_PIECE]=Get_Random_Hashkey_Value();
            hashkey_piece[i*10+j][GOLD|DOG_PIECE]=Get_Random_Hashkey_Value();
            hashkey_piece[i*10+j][GOLD|CAT_PIECE]=Get_Random_Hashkey_Value();
            hashkey_piece[i*10+j][GOLD|RABBIT_PIECE]=Get_Random_Hashkey_Value();
            hashkey_piece[i*10+j][SILVER|ELEPHANT_PIECE]=Get_Random_Hashkey_Value();
            hashkey_piece[i*10+j][SILVER|CAMEL_PIECE]=Get_Random_Hashkey_Value();
            hashkey_piece[i*10+j][SILVER|HORSE_PIECE]=Get_Random_Hashkey_Value();
            hashkey_piece[i*10+j][SILVER|DOG_PIECE]=Get_Random_Hashkey_Value();
            hashkey_piece[i*10+j][SILVER|CAT_PIECE]=Get_Random_Hashkey_Value();
            hashkey_piece[i*10+j][SILVER|RABBIT_PIECE]=Get_Random_Hashkey_Value();
        }
    }
    for (i=0; i<4; i++)
    {
        hashkey_step[i]=Get_Random_Hashkey_Value();
    }
    hashkey_at_move[GOLD]=Get_Random_Hashkey_Value();
    hashkey_at_move[SILVER]=Get_Random_Hashkey_Value();
    HASH_Set_Size(0);
}

void HASH_Set_Size(unsigned long long int size)
{
	unsigned long long int entries;
	unsigned long long int two_power=0;
	
	free((void *) hash_table);
	entries=size/sizeof(hash_entry_t);
	if (entries==0)
	{
		hash_table=NULL;
		sprintf(message,"Hash-table turned off.\n");
		BOARD_Message();
	} else
	{
		while (entries!=1)
		{
			entries>>=1;
			two_power++;
		}
		entries<<=two_power;
		hash_mask=entries-1;
		sprintf(message,"Entries in hashtable : %d\n",(unsigned long int)entries);
		BOARD_Message();
		sprintf(message,"Hash mask : %#X\n",(unsigned long int)hash_mask);
		BOARD_Message();
		hash_table=(hash_entry_t *) calloc(entries,sizeof(hash_entry_t));
		if (hash_table==NULL)
		{
			sprintf(message,"Error when allocating hash-table, probably out of memory.\n");
			BOARD_Message();
		} else
		{
			sprintf(message,"Memory reserved : %d\n",(unsigned long int)entries*sizeof(hash_entry_t));
			BOARD_Message();
		}
	}
}

void HASH_Put_Entry(unsigned long long int hashkey, int value, int depth, unsigned char type, move_t *mp)
{
	hash_entry_t *hp;
    int i;
    
	if (hash_table!=NULL)
	{
    	hp=hash_table+(hashkey&hash_mask);
    	hp->hashkey=hashkey;
    	hp->value=value;
    	hp->depth=depth;
    	hp->type=type;
    	if (mp==NULL) // We're not storing a move for this position
    	{
            hp->move=FALSE;
        } else // We are storing a move for this position
        {
            hp->move=TRUE;
            hp->steps=mp->steps;
            hp->pass=mp->pass;
            for (i=0; i<2; i++)
            {
                hp->piece[i]=mp->piece[i];
                hp->from[i]=mp->from[i];
                hp->to[i]=mp->to[i];
                hp->capture_square[i]=mp->capture_square[i];
                hp->captured_piece[i]=mp->captured_piece[i];
            }
        }
    }
}

hash_entry_t *HASH_Get_Entry(board_t *board)
{
	hash_entry_t *hash_table_pos;
	
	if (hash_table==NULL)
	{
		hash_table_pos=NULL;
	} else
	{
        hash_table_pos=hash_table+(board->hashkey&hash_mask);
    	if (board->hashkey!=hash_table_pos->hashkey)
    	{
            hash_table_pos=NULL;
        }
    }
    return hash_table_pos;
}

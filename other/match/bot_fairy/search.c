#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include <time.h>

#include "search.h"
#include "board.h"
#include "eval.h"
#include "hash.h"

#define MAX_PV_LENGTH 100

#define ROOT_WINDOW 1000

#define DEBUG_DEPTH -1
// #define DEBUG_SEARCH  // If this is defined, then debugging information will be printed about the alpha-beta search

#define HASH_MOVES // If this is defined, then hash moves are used in move ordering

// different sources for pre-moves, to help in gathering statistics
#define PV_MOVE 1
#define HASH_MOVE 2
#define KILLER_MOVE 3
#define SHALLOW_MOVE 4

static unsigned long long int nodes, total_nodes;
static unsigned long long int evals, total_evals;
static unsigned long long int sum_eval_depths;
static int depth, max_depth;
static move_t pv[MAX_PV_LENGTH][MAX_PV_LENGTH];
static int pv_length[MAX_PV_LENGTH];
static int hash_hit[MAX_PV_LENGTH];
static unsigned long long int pv_position[MAX_PV_LENGTH];
static int pv_positions;
static move_t pv_move[MAX_PV_LENGTH];
static long long int start_time;
static long long int allocated_time;
static unsigned long long int start_hashkey;
static move_t killer_move[MAX_PV_LENGTH];
static int killer_move_set[MAX_PV_LENGTH];
static int disturbance;
static unsigned long long int hash_writes_lower_bound, hash_writes_upper_bound, hash_writes_exact_value;
static unsigned long long int hash_hits, hash_value_used;
static unsigned long long int hash_moves, hash_moves_illegal, hash_moves_good, hash_moves_cutoff;
static unsigned long long int pv_moves, pv_moves_illegal, pv_moves_good, pv_moves_cutoff;
static unsigned long long int killer_moves, killer_moves_illegal, killer_moves_good, killer_moves_cutoff;
static unsigned long long int shallow_moves, shallow_moves_illegal, shallow_moves_good, shallow_moves_cutoff;
static unsigned long long int scout_searches, scout_researches;

//  static move_t move_seq[MAX_PV_LENGTH];  // used for tracing the sequences of moves done to reach a node

void Generate_Hash_Move(hash_entry_t *hp, move_t *mp);

void Show_Search_Info(board_t *bp, int ply_finished, int depth, int value)
{
    long long int elapsed_time;
    int pv_steps[MAX_PV_LENGTH];
    int i;
    hash_entry_t *hp;
    move_t hash_move;
    int tracing_pv=TRUE;
    
    elapsed_time=clock()-start_time;
    if (disturbance)
    {
        sprintf(message,"~~~%3d/%4.1f/%2d",depth,(double)sum_eval_depths/evals,max_depth);
        BOARD_Message();
    } else
    {
        if (ply_finished)
        {
            if (evals==0)
            {
                sprintf(message,"===%3d/    /  ",depth,(double)sum_eval_depths/evals,max_depth);
                BOARD_Message();
            } else
            {
                sprintf(message,"===%3d/%4.1f/%2d",depth,(double)sum_eval_depths/evals,max_depth);
                BOARD_Message();
            }
        } else
        {
            sprintf(message,"   %3d........",depth);
            BOARD_Message();
        }
    }
    sprintf(message,"%7d %9d %9d",value,(long int)(total_nodes+nodes),(long int)(total_evals+evals));
    BOARD_Message();
    sprintf(message," %6.2f ",(double)(elapsed_time)/CLOCKS_PER_SEC);
    BOARD_Message();
    pv_steps[0]=1;
    for (i=0; i<pv_length[0]; i+=pv[0][i].steps)
    {
        if (hash_hit[0]==i)
        {
            sprintf(message,"|");
            BOARD_Message();
        } else
        {
            sprintf(message," ");
            BOARD_Message();
        }
        BOARD_Print_Move(&pv[0][i]);
        BOARD_Do_Move(bp,&pv[0][i]);
        pv_steps[i+pv[0][i].steps]=pv[0][i].steps;
    }
    if (hash_hit[0]==i)
    {
        sprintf(message,"|");
        BOARD_Message();
    }
    // Try tracing PV through hash table        
    while (tracing_pv && i<20)
    {
        tracing_pv=FALSE;
        hp=HASH_Get_Entry(bp);
        if (hp!=NULL && hp->move)
        {
            Generate_Hash_Move(hp,&hash_move);
            if (Check_Legal_Move(bp,&hash_move)) // Check if it is a legal move, and if so, add it to list of pre-moves.
            {
                tracing_pv=TRUE;
                if (i==pv_length[0])
                {
                    sprintf(message,":");
                    BOARD_Message();
                } else
                {
                    sprintf(message," ");
                    BOARD_Message();
                }
                BOARD_Copy_Move(&hash_move,&pv[0][i]);
                BOARD_Print_Move(&pv[0][i]);
                BOARD_Do_Move(bp,&pv[0][i]);
                pv_steps[i+pv[0][i].steps]=pv[0][i].steps;
                i+=pv[0][i].steps;
            }
        }
    }
    for (i-=pv_steps[i]; i>=0; i-=pv_steps[i])
    {
        BOARD_Undo_Move(bp,&pv[0][i]);
    }
    sprintf(message,"\n");
    BOARD_Message();
}

int SEARCH_Start_Search(board_t *bp, move_t ml[4], test_data_t *tp)
{
    int i, j, k;
    int steps=0;
    int value, old_value;
    int pv_steps[MAX_PV_LENGTH];
    long long int elapsed_time;
    long long int remaining_time;
    int number_of_steps;
    int depth_limit;
        
    start_hashkey=bp->hashkey^hashkey_at_move[GOLD]^hashkey_at_move[SILVER];
    if (tp!=NULL)
    {
        remaining_time=(long long int)18000*(long long int)CLOCKS_PER_SEC; // 5 hours - if a position isn't done by then, something is broken
        number_of_steps=1;
        depth_limit=tp->depth_limit; // should be assigned depending upon the position
    } else
    {
        #ifdef TESTING
        {
            remaining_time=(long long int)5*(long long int)CLOCKS_PER_SEC; // 2 minutes per move            
        }
        #else
        {
            remaining_time=(long long int)3*(long long int)CLOCKS_PER_SEC; // 2 minutes per move
        }
        #endif
        number_of_steps=4;
        depth_limit=500;
    }
    for (i=0; steps<number_of_steps; i++)
    {
        start_time=clock();
        if (tp!=NULL)
        {
            allocated_time=remaining_time;
        } else
        {
            switch (steps) // Allocate time depending upon how many steps are left to do
            {
                case 0 :
                    allocated_time=remaining_time*50/100; // 40% for the first step
                    break;
                case 1 :
                    allocated_time=remaining_time*60/100; // 50% of remaining for second step
                    break;
                case 2 :
                    allocated_time=remaining_time*70/100; // 60% of remaining for third step
                    break;
                case 3 :
                    allocated_time=remaining_time; // all remaining for fourth step
                    break;
                default :
                    allocated_time=remaining_time/4;
                    sprintf(message,"Illegal step number when trying to allocate time!!!\n");
                    BOARD_Message();
                    break;
            }
        }
        sprintf(message,"\nSearching position:\n");
        BOARD_Message();
        BOARD_Print_Position(bp);
        sprintf(message,"\nRemaining time: %6.2f",(double)remaining_time/CLOCKS_PER_SEC);
        BOARD_Message();
        sprintf(message,"\nAllocated time: %6.2f",(double)allocated_time/CLOCKS_PER_SEC);
        BOARD_Message();
        sprintf(message,"\n         Depth  Value     Nodes     Evals   Time  PV\n");
        BOARD_Message();
        total_nodes=0;
        total_evals=0;
        elapsed_time=0;
        hash_writes_lower_bound=0;
        hash_writes_upper_bound=0;
        hash_writes_exact_value=0;
        hash_hits=0;
        hash_moves=0;
        hash_moves_illegal=0;
        hash_moves_good=0;
        hash_moves_cutoff=0;
        pv_moves=0;
        pv_moves_illegal=0;
        pv_moves_good=0;
        pv_moves_cutoff=0;
        killer_moves=0;
        killer_moves_illegal=0;
        killer_moves_good=0;
        killer_moves_cutoff=0;
        shallow_moves=0;
        shallow_moves_illegal=0;
        shallow_moves_good=0;
        shallow_moves_cutoff=0;
        hash_value_used=0;
        scout_searches=0;
        scout_researches=0;
        disturbance=FALSE;
        for(j=0; j<MAX_PV_LENGTH; j++)
        {
            killer_move_set[j]=FALSE;
        }
        for (j=5; elapsed_time<allocated_time/2 && j<=depth_limit; j+=5)
        {
            depth=0;
            max_depth=0;
            sum_eval_depths=0;
            nodes=0;
            evals=0;
            if (j>5)
            {
                value=Nega_Max(bp,old_value-ROOT_WINDOW,old_value+ROOT_WINDOW,j);
                if (value<=old_value-ROOT_WINDOW) // Fail-low
                {
                    sprintf(message,"---\n");
                    BOARD_Message();
                    value=Nega_Max(bp,-999999,old_value-ROOT_WINDOW+1);
                } else if (value>=old_value+ROOT_WINDOW)
                {
                    sprintf(message,"+++\n");
                    BOARD_Message();
                    value=Nega_Max(bp,old_value+ROOT_WINDOW-1,999999);
                }
            } else
            {
                value=Nega_Max(bp,-999999,999999,j);
            }
            old_value=value;
            if (value==500000-bp->move*4-bp->steps) // We've already won in the root position, so need just to pass
            {
                pv[0][0].pass=TRUE;
                pv[0][0].steps=4-bp->steps;
                pv_length[0]=1;
                hash_hit[0]=-1;
            }
            elapsed_time=clock()-start_time;
            Show_Search_Info(bp,TRUE,j,value);
            total_nodes+=nodes;
            total_evals+=evals;
            pv_positions=0; // Store hashvalues for pv in pv_position[] - pv_positions keep count of how many we have stored.
            pv_steps[0]=1;
            for (k=0; k<pv_length[0]; k+=pv[0][k].steps) 
            {
                pv_position[pv_positions]=bp->hashkey;
                BOARD_Copy_Move(&pv[0][k],&pv_move[pv_positions]);
                pv_positions++;
                BOARD_Do_Move(bp,&pv[0][k]);
                pv_steps[k+pv[0][k].steps]=pv[0][k].steps;
            }
            for (k-=pv_steps[k]; k>=0; k-=pv_steps[k])
            {
                BOARD_Undo_Move(bp,&pv[0][k]);
            }
            BOARD_Copy_Move(&pv[0][0],&ml[i]);
            #ifdef DEBUG_SEARCH
            {
//               system("pause");
            }
            #endif
        }
        if (tp!=NULL) // we're interested in the test data
        {
            tp->time+=(double)(elapsed_time)/CLOCKS_PER_SEC;
            tp->nodes+=total_nodes;
            tp->evals+=total_evals;
        }
    	sprintf(message,"Hash writes : %d  ",(unsigned long int)(hash_writes_upper_bound+hash_writes_lower_bound+hash_writes_exact_value));
    	BOARD_Message();
        sprintf(message,"(%d upper-bounded values, %d lower-bounded values, %d exact values)\n",(unsigned long int)hash_writes_upper_bound,(unsigned long int)hash_writes_lower_bound,(unsigned long int)hash_writes_exact_value);
        BOARD_Message();
        sprintf(message,"Hash hits : %d  ",(unsigned long int)hash_hits);
        BOARD_Message();
        if (hash_hits!=0)
        {
            sprintf(message,"Hash values used : %d (%2.1f%%)\n",(unsigned long int)hash_value_used,(double)hash_value_used/hash_hits*100);
            BOARD_Message();
        } else
        {
            sprintf(message,"Hash values used : %d (%2.1f%%)\n",(unsigned long int)hash_value_used,0);
            BOARD_Message();
        }
        sprintf(message,"PV moves tried: %d  Invalid PV moves: %d  Good PV moves: %d  Cutoff PV moves: %d\n",(unsigned long int)pv_moves,(unsigned long int)pv_moves_illegal,(unsigned long int)pv_moves_good,(unsigned long int)pv_moves_cutoff);
        BOARD_Message();
        sprintf(message,"Killer moves tried: %d  Invalid killer moves: %d  Good killer moves: %d  Cutoff killer moves: %d\n",(unsigned long int)killer_moves,(unsigned long int)killer_moves_illegal,(unsigned long int)killer_moves_good,(unsigned long int)killer_moves_cutoff);
        BOARD_Message();
        sprintf(message,"Hash moves tried: %d  Invalid hash moves: %d  Good hash moves: %d  Cutoff hash moves: %d\n",(unsigned long int)hash_moves,(unsigned long int)hash_moves_illegal,(unsigned long int)hash_moves_good,(unsigned long int)hash_moves_cutoff);
        BOARD_Message();
        sprintf(message,"Shallow moves tried: %d  Invalid shallow moves: %d  Good shallow moves: %d  Cutoff shallow moves: %d\n",(unsigned long int)shallow_moves,(unsigned long int)shallow_moves_illegal,(unsigned long int)shallow_moves_good,(unsigned long int)shallow_moves_cutoff);
        BOARD_Message();
        sprintf(message,"Scout searches done: %d  Researches done: %d (%2.1f%%)\n",(unsigned long int)scout_searches,(unsigned long int)scout_researches,(double)scout_researches/scout_searches*100);
        BOARD_Message();
        remaining_time-=elapsed_time;
        steps+=ml[i].steps;
        BOARD_Do_Move(bp,&ml[i]);
    }
    i--;
    for (; i>=0; i--)
    {
        BOARD_Undo_Move(bp,&ml[i]);
    }
    sprintf(message,"\n");
    BOARD_Message();
    return value;
}

int Check_Extensions(board_t *bp, move_t *mp)
{
    int steps_change=0;
#define SELF_CAPTURE -5 // how much to extend for self-captures: should be negative or 0.
#define CAPTURE 5 // how much to extend for capturing an enemy piece: should be positive or 0.
#define PUSH_PULL  5 // how much to extend for pushing or pulling moves: should be positive or 0.

    if (mp->pass)
    {
        // Pass - do no extensions
    } else
    {
        if (mp->capture_square[0]) // Capture
        {
            if ((mp->captured_piece[0] & OWNER_MASK)==bp->at_move) // Self-capture, search shallower
            {
                steps_change+=SELF_CAPTURE;
            } else // capture of other side's piece, search deeper
            {
                steps_change+=CAPTURE;
            }
        }
        if (mp->steps==2) // Push or pull, search deeper
        {
            steps_change+=PUSH_PULL;
            if (mp->capture_square[1]) // Capture
            {
                if ((mp->captured_piece[1]& OWNER_MASK)==bp->at_move) // Self-capture, search shallower
                {
                    steps_change+=SELF_CAPTURE;
                } else // capture of other side's piece, search deeper
                {
                    steps_change+=CAPTURE;
                }
            }
        } else if (PIECE(mp->from[0])==RABBIT_PIECE) // if rabbit advances, extend deeper
        {
            if (OWNER(mp->from[0])==GOLD) // gold rabbit
            {
                if (mp->to[0]-mp->from[0]==NORTH) // gold rabbit advances
                {
                    if (ROW(mp->from[0])==6)
                    {
                        steps_change+=6;
                    } else if (ROW(mp->from[0])==5)
                    {
                        steps_change+=3;
                    } else if (ROW(mp->from[0])==4)
                    {
                        steps_change+=1;
                    }
                    
                } else // rabbit sidesteps
                {
                    if (ROW(mp->from[0])==7)
                    {
                        steps_change+=5;
                    } else if (ROW(mp->from[0])==6)
                    {
                        steps_change+=2;
                    }
                }
            } else // silver rabbit
            {
                if (mp->to[0]-mp->from[0]==SOUTH) // silver rabbit advances
                {
                    if (ROW(mp->from[0])==3)
                    {
                        steps_change+=6;
                    } else if (ROW(mp->from[0])==4)
                    {
                        steps_change+=3;
                    } else if (ROW(mp->from[0])==5)
                    {
                        steps_change+=1;
                    }
                    
                } else // rabbit sidesteps
                {
                    if (ROW(mp->from[0])==2)
                    {
                        steps_change+=6;
                    } else if (ROW(mp->from[0])==3)
                    {
                        steps_change+=1;
                    }
                }
            }
        }
    }
    return steps_change;
}

int Check_Legal_Move(board_t *bp, move_t *mp)
{
    int valid_move=FALSE;
    int trap_index;
    int capture;
    int i;
    int capture_square[2],captured_piece[2];
    int frozen;
    int adjacent_friend,adjacent_stronger_enemy;
    
    if (mp->pass)
    {
        if (bp->steps>0 && bp->steps+mp->steps==4)
        {
            valid_move=TRUE;
        }
    } else
    {
        if (BOARD(mp->from[0])==mp->piece[0] && BOARD(mp->to[0])==EMPTY_SQUARE) // First piece movement is valid
        {
            frozen=FALSE;
            if ((mp->piece[0] & OWNER_MASK)==bp->at_move) // our piece we're moving - not a pushed/pulled piece - so need to check so that it is not frozen
            {
                // check so that piece is not frozen!
                adjacent_friend=FALSE;
                adjacent_stronger_enemy=FALSE;
                for (i=0; i<4; i++) // check if piece is frozen
                {
                    if (OWNER(mp->from[0]+direction[i])==bp->at_move) // is a friendly piece adjacent?
                    {
                        adjacent_friend=TRUE;
                    } else if (OWNER(mp->from[0]+direction[i])==(bp->at_move^FLIP_SIDE) // is an enemy piece adjacent
                        && PIECE(mp->from[0]+direction[i])>PIECE(mp->from[0])) // and stronger than this piece?
                    {
                        adjacent_stronger_enemy=TRUE;
                    }
                }
                frozen=adjacent_stronger_enemy && !adjacent_friend;
            }
            if (!frozen)
            {
                BOARD(mp->to[0])=mp->piece[0]; // temporarily move first piece.
                BOARD(mp->from[0])=EMPTY_SQUARE;
                // check first capture!
                capture_square[0]=0;
                for (trap_index=0; trap_index<4; trap_index++) // check each of the four trap squares
                {
                    if (BOARD(trap[trap_index])!=EMPTY_SQUARE)
                    {
                        capture=TRUE;
                        for (i=0; i<4; i++)
                        {
                            if (OWNER(trap[trap_index])==OWNER(trap[trap_index]+direction[i]))
                            {
                                capture=FALSE;
                            }
                        }
                        if (capture)
                        {
                            capture_square[0]=trap[trap_index];
                            captured_piece[0]=BOARD(trap[trap_index]);
                            BOARD(trap[trap_index])=EMPTY_SQUARE; // temporarily capture the piece, to avoid detecting same capture after step 2
                        }
                    }
                }
                if (mp->capture_square[0]==capture_square[0] && (capture_square[0]==0 || mp->captured_piece[0]==captured_piece[0])) // capture matches
                {
                    if (mp->steps==2) // 2-step move, need to check second part of it.
                    {
                        if (BOARD(mp->from[1])==mp->piece[1]) // Second piece is valid
                        {
                            frozen=FALSE;
                            if ((mp->piece[1] & OWNER_MASK)==bp->at_move) // our piece we're moving - not a pushed/pulled piece - so need to check so that it is not frozen
                            {
                                // check so that piece is not frozen!
                                adjacent_friend=FALSE;
                                adjacent_stronger_enemy=FALSE;
                                for (i=0; i<4; i++) // check if piece is frozen
                                {
                                    if (OWNER(mp->from[1]+direction[i])==bp->at_move) // is a friendly piece adjacent?
                                    {
                                        adjacent_friend=TRUE;
                                    } else if (OWNER(mp->from[1]+direction[i])==(bp->at_move^FLIP_SIDE) // is an enemy piece adjacent
                                        && PIECE(mp->from[1]+direction[i])>PIECE(mp->from[1])) // and stronger than this piece?
                                    {
                                        adjacent_stronger_enemy=TRUE;
                                    }
                                }
                                frozen=adjacent_stronger_enemy && !adjacent_friend;
                            }
                            if (!frozen)
                            {
                                BOARD(mp->to[1])=mp->piece[1]; // temporarily move second piece.
                                BOARD(mp->from[1])=EMPTY_SQUARE;
                                // check second capture!
                                capture_square[1]=0;
                                for (trap_index=0; trap_index<4; trap_index++) // check each of the four trap squares
                                {
                                    if (BOARD(trap[trap_index])!=EMPTY_SQUARE)
                                    {
                                        capture=TRUE;
                                        for (i=0; i<4; i++)
                                        {
                                            if (OWNER(trap[trap_index])==OWNER(trap[trap_index]+direction[i]))
                                            {
                                                capture=FALSE;
                                            }
                                        }
                                        if (capture)
                                        {
                                            capture_square[1]=trap[trap_index];
                                            captured_piece[1]=BOARD(trap[trap_index]);
                                        }
                                    }
                                }
                                if (mp->capture_square[1]==capture_square[1] && (capture_square[1]==0 || mp->captured_piece[1]==captured_piece[1])) // capture matches
                                {
                                    valid_move=TRUE;
                                }
                                BOARD(mp->from[1])=mp->piece[1]; // Undo temporary movement of second piece.
                                BOARD(mp->to[1])=EMPTY_SQUARE;
                            }
                        }
                    } else
                    { // 1-step move, and it hasn't failed yet, thus must be a valid move.
                        valid_move=TRUE;
                    }
                }
                if (capture_square[0]) // If a temporary first capture was done, undo it.
                {
                    BOARD(capture_square[0])=captured_piece[0];
                }
                BOARD(mp->from[0])=mp->piece[0]; // Undo temporary movement of first piece.
                BOARD(mp->to[0])=EMPTY_SQUARE;
            }
        }
    }
    return valid_move;
}

void Generate_Hash_Move(hash_entry_t *hp, move_t *mp)
{
    int i;
    
    mp->steps=hp->steps;
    mp->pass=hp->pass;
    for (i=0; i<2; i++)
    {
        mp->piece[i]=hp->piece[i];
        mp->to[i]=hp->to[i];
        mp->from[i]=hp->from[i];
        mp->capture_square[i]=hp->capture_square[i];
        mp->captured_piece[i]=hp->captured_piece[i];
    }
    if (mp->pass)
    {
        mp->hashkey=0x0ULL;
    } else
    {
        mp->hashkey=hashkey_piece[mp->from[0]][mp->piece[0]]^hashkey_piece[mp->to[0]][mp->piece[0]];
        if (mp->capture_square[0])
        {
            mp->hashkey^=hashkey_piece[mp->capture_square[0]][mp->captured_piece[0]];
        }
        if (mp->steps==2)
        {
            mp->hashkey^=hashkey_piece[mp->from[1]][mp->piece[1]]^hashkey_piece[mp->to[1]][mp->piece[1]];
            if (mp->capture_square[1])
            {
                mp->hashkey^=hashkey_piece[mp->capture_square[1]][mp->captured_piece[1]];
            }
        }
    }
}

int Nega_Max(board_t *bp, int lower_bound, int upper_bound, int steps)
{
    int value;
    int number_of_moves;
    move_t move_list[MAX_NUMBER_MOVES];
    move_t pre_move_list[MAX_NUMBER_MOVES];
    move_t *current_move;
    int pre_moves=0;
    int pre_moves_done=FALSE;
    int already_tried=FALSE;
    int shallow_move_found=FALSE;
    int i, j;
    int cutoff=FALSE;
    int steps_change;
    int original_lower_bound, original_upper_bound;
    int terminal_node=FALSE;
    hash_entry_t *hp;
    move_t hash_move;
    int pre_moves_source[MAX_NUMBER_MOVES];
    int pv_move_found=FALSE;
        
    nodes++;
    original_lower_bound=lower_bound; // need these when writing position to hashtable to know if we're writing a lower, upper or exact value
    original_upper_bound=upper_bound;
    if (nodes%1000==0) // Every 10000 nodes, check for disturbances.
    {
        if (clock()-start_time>allocated_time) // We've used up our allocated time, exit search!
        {
            disturbance=TRUE;
        }
    }
    if (!disturbance)
    {
        hp=HASH_Get_Entry(bp);
        if (hp!=NULL && (hp->move || depth>0)) // Check if position is in hash table.
        {
            hash_hits++;            
            // Check if we can use value from hash table.
			if (hp->depth>=steps)
			{
				switch (hp->type)
				{
					case EXACT_VALUE :
						hash_value_used++;
                        pv_length[depth]=0;
                        hash_hit[depth]=0;
                        if (hp->move) // If we have a hash move, generate the actual move from the info stored in the hashtable.
                        {
                            Generate_Hash_Move(hp,&hash_move);
                            BOARD_Copy_Move(&hash_move,&pv[depth][depth]);
                            pv_length[depth]=hash_move.steps;
                        }
                        #ifdef DEBUG_SEARCH
                        {
                            int spaces;
                            
                            if (depth<=DEBUG_DEPTH)
                            {
                                for (spaces=0; spaces<depth; spaces++) 
                                {
                                    sprintf(message,"    ");
                                    BOARD_Message();
                                }
                                sprintf(message,"%d is exact value (hash depth %d) from hash at depth %d.\n",hp->value,hp->depth,depth);
                                BOARD_Message();
                            }
                        }
                        #endif
						return hp->value;
						break;
					case LOWER_BOUND :
						if (hp->value>=upper_bound)
						{
							hash_value_used++;
                            pv_length[depth]=0;
                            hash_hit[depth]=0;
                            #ifdef DEBUG_SEARCH
                            {
                                int spaces;
                                
                                if (depth<=DEBUG_DEPTH)
                                {
                                    for (spaces=0; spaces<depth; spaces++) 
                                    {
                                        sprintf(message,"    ");
                                        BOARD_Message();
                                    }
                                    sprintf(message,"%d is lower-bound value (hash depth %d) from hash at depth %d.\n",hp->value,hp->depth,depth);
                                    BOARD_Message();
                                }
                            }
                            #endif
							return hp->value;
						}
						break;
					case UPPER_BOUND :
						if (hp->value<=lower_bound)
						{
							hash_value_used++;
	                        pv_length[depth]=0;
	                        hash_hit[depth]=0;
                            #ifdef DEBUG_SEARCH
                            {
                                int spaces;
                                
                                if (depth<=DEBUG_DEPTH)
                                {
                                    for (spaces=0; spaces<depth; spaces++) 
                                    {
                                        sprintf(message,"    ");
                                        BOARD_Message();
                                    }
                                    sprintf(message,"%d is upper-bound value (hash depth %d) from hash at depth %d.\n",hp->value,hp->depth,depth);
                                    BOARD_Message();
                                }
                            }
                            #endif
    						return hp->value;
						}
						break;
				}
			}			
        }
        // Check to see if either player has won the game.
        for (i=A8; i<=H8; i+=EAST)
        {
            if (BOARD(i)==(GOLD | RABBIT_PIECE))
            {
                if (bp->at_move==GOLD)
                {
                    lower_bound=500000-bp->move*4-bp->steps;
                } else
                {
                    lower_bound=-500000+bp->move*4+bp->steps;
                }
                cutoff=TRUE;
            }
        }
        for (i=A1; i<=H1; i+=EAST)
        {
            if (BOARD(i)==(SILVER | RABBIT_PIECE))
            {
                if (bp->at_move==GOLD)
                {
                    lower_bound=-500000+bp->move*4+bp->steps;
                } else
                {
                    lower_bound=500000-bp->move*4-bp->steps;
                }
                cutoff=TRUE;
            }
        }
        if (cutoff)
        {
            pv_length[depth]=0;
            hash_hit[depth]=-1;
            if (depth>max_depth) max_depth=depth;
            terminal_node=TRUE;
            #ifdef DEBUG_SEARCH
            {
                int spaces;
                
                if (depth<=DEBUG_DEPTH)
                {
                    for (spaces=0; spaces<depth; spaces++) 
                    {
                        sprintf(message,"    ");
                        BOARD_Message();
                    }
                    sprintf(message,"%d is value due to win by goal at depth %d.\n",lower_bound,depth);
                    BOARD_Message();
                }
            }
            #endif
        } else
        {
            if (steps<=0)
            {
                pv_length[depth]=0;
                hash_hit[depth]=-1;
                evals++;
                if (depth>max_depth) max_depth=depth;
                sum_eval_depths+=depth;
                lower_bound=EVAL_Eval(bp,FALSE);
                terminal_node=TRUE;
                #ifdef DEBUG_SEARCH
                {
                    int spaces;
                    
                    if (depth<=DEBUG_DEPTH)
                    {
                        for (spaces=0; spaces<depth; spaces++) 
                        {
                            sprintf(message,"    ");
                            BOARD_Message();
                        }
                        sprintf(message,"%d is value from Eval at depth %d.\n",lower_bound,depth);
                        BOARD_Message();
                    }
                }
                #endif
            } else
            {
                // Check if position is in PV - if so, add PV move to list of moves to try pre-generating movelist.
                for (i=0; i<pv_positions; i++)
                {
                    if (bp->hashkey==pv_position[i])
                    {
                        pv_moves++;
                        if (Check_Legal_Move(bp,&pv_move[i]))
                        {
                            #ifdef DEBUG_SEARCH
                            {
                                int spaces;
                                
                                if (depth<=DEBUG_DEPTH)
                                {
                                    for (spaces=0; spaces<depth; spaces++) 
                                    {
                                        sprintf(message,"    ");
                                        BOARD_Message();
                                    }
                                    BOARD_Print_Move(&pv_move[i]);
                                    sprintf(message," is move from pv at depth %d.\n",depth);
                                    BOARD_Message();
                                }
                            }
                            #endif
                            BOARD_Copy_Move(&pv_move[i],&pre_move_list[pre_moves]);
                            pre_moves_source[pre_moves]=PV_MOVE;
                            pre_moves++;
                            pv_move_found=TRUE;
                        } else
                        {
                            pv_moves_illegal++;
                        }
                    }
                }
                // Check if killer move is legal move - if so, add killer move to list of pre-moves.
                // Strangely enough, trying killer moves before hash moves resulted in quicker search... keep an eye on this and try it again as other things change.
                if (killer_move_set[depth])
                {
                    killer_moves++;
                    if (Check_Legal_Move(bp,&killer_move[depth]))
                    {
                        #ifdef DEBUG_SEARCH
                        {
                            int matching;
                            int spaces;
    
                            if (depth<=DEBUG_DEPTH)
                            {
                                for (spaces=0; spaces<depth; spaces++) 
                                {
                                    sprintf(message,"    ");
                                    BOARD_Message();
                                }
                                BOARD_Print_Move(&killer_move[depth]);
                                sprintf(message," is move from killer moves at depth %d.\n",depth);
                                BOARD_Message();
                            }
                            number_of_moves=BOARD_Generate_Moves(bp,move_list);
                            matching=FALSE;
                            for (i=0; i<number_of_moves; i++)
                            {
                                if (killer_move[depth].hashkey==move_list[i].hashkey)
                                {
                                    matching=TRUE;
                                }
                            }
                            if (!matching)
                            {
                                sprintf(message,"Illegal killer move validated as legal move in position:\n");
                                BOARD_Message();
                                BOARD_Print_Position(bp);
                                sprintf(message,"The illegal killer move is : ");
                                BOARD_Message();
                                BOARD_Print_Move(&killer_move[depth]);
                                sprintf(message,"\n");
                                BOARD_Message();
                            }
                        }
                        #endif
                        BOARD_Copy_Move(&killer_move[depth],&pre_move_list[pre_moves]);
                        pre_moves_source[pre_moves]=KILLER_MOVE;
                        pre_moves++;
                    } else
                    {
                        killer_moves_illegal++;
                    }
                }
                #ifdef HASH_MOVES
                {
                    // Check if we have a legal hash move - if so, add hash move to list of pre-moves.
                    if (hp!=NULL)
                    {
                        if (hp->move) // If we have a hash move, generate the actual move from the info stored in the hashtable.
                        {
                            Generate_Hash_Move(hp,&hash_move);
                            hash_moves++;
                            if (Check_Legal_Move(bp,&hash_move)) // Check if it is a legal move, and if so, add it to list of pre-moves.
                            {
                                #ifdef DEBUG_SEARCH
                                {
                                    int matching;
                                    int spaces;
                                    
                                    if (depth<=DEBUG_DEPTH)
                                    {
                                        for (spaces=0; spaces<depth; spaces++) 
                                        {
                                            sprintf(message,"    ");
                                            BOARD_Message();
                                        }
                                        BOARD_Print_Move(&hash_move);
                                        sprintf(message," is move from hash table at depth %d.\n",depth);
                                        BOARD_Message();
                                    }
                                    number_of_moves=BOARD_Generate_Moves(bp,move_list);
                                    matching=FALSE;
                                    for (i=0; i<number_of_moves; i++)
                                    {
                                        if (hash_move.hashkey==move_list[i].hashkey)
                                        {
                                            matching=TRUE;
                                        }
                                    }
                                    if (!matching)
                                    {
                                        sprintf(message,"Illegal hash move validated as legal move in position:\n");
                                        BOARD_Message();
                                        BOARD_Print_Position(bp);
                                        sprintf(message,"The illegal hash move is : ");
                                        BOARD_Message();
                                        BOARD_Print_Move(&hash_move);
                                        sprintf(message,"\n");
                                        BOARD_Message();
                                    }
                                }
                                #endif
                                BOARD_Copy_Move(&hash_move,&pre_move_list[pre_moves]);
                                pre_moves_source[pre_moves]=HASH_MOVE;
                                pre_moves++;
                            } else
                            {
                                hash_moves_illegal++;
                            }
                        }
                    }
                }
                #endif
                number_of_moves=pre_moves; // We have at least this many moves available.
                for (i=0; !disturbance && !cutoff && (!pre_moves_done || i<number_of_moves); i++)
                {
                    if (!pre_moves_done && i==pre_moves) // done with checking pre-moves - generate the rest of the moves and start on them
                    {
                        pre_moves_done=TRUE;
                        // Try shallow search for better move ordering / hash moves
                        if (!shallow_move_found && steps>50 && lower_bound==original_lower_bound && !pv_move_found && pre_moves<=1)
                        {
                            Nega_Max(bp,lower_bound,upper_bound,steps/6);
                            // After doing the shallow search, the best move (if one was found) should be stored in the hash table
                            hp=HASH_Get_Entry(bp);
                            if (hp!=NULL)
                            {
                                if (hp->move) // We have a hash move, generate the actual move from the info stored in the hashtable.
                                {
                                    Generate_Hash_Move(hp,&hash_move);
                                    shallow_moves++;
                                    if (Check_Legal_Move(bp,&hash_move)) // Check if it is a legal move, and if so, add it to list of pre-moves.
                                    {
                                        BOARD_Copy_Move(&hash_move,&pre_move_list[pre_moves]);
                                        pre_moves_source[pre_moves]=SHALLOW_MOVE;
                                        pre_moves++;
                                        number_of_moves=pre_moves; // We have at least this many moves available.
                                        shallow_move_found=TRUE;
                                        pre_moves_done=FALSE;
                                        i--;
                                    } else
                                    {
                                        shallow_moves_illegal++;
                                    }
                                }
                            }
                        }
                        if (pre_moves_done)
                        {
                            i=-1;
                            number_of_moves=BOARD_Generate_Moves(bp,move_list); // Generate proper movelist                            
                        }
                    } else
                    {
                        already_tried=FALSE;  // Checking to see if we've already tried this move
                        if (pre_moves_done)
                        {
                            current_move=&move_list[i];
                            for (j=0; !already_tried && j<pre_moves; j++)
                            {
                                if (current_move->hashkey==pre_move_list[j].hashkey)
                                {
                                    already_tried=TRUE;
                                }
                            }
                        } else
                        {
                            current_move=&pre_move_list[i];
                            for (j=0; !already_tried && j<i; j++)
                            {
                                if (current_move->hashkey==pre_move_list[j].hashkey)
                                {
                                    already_tried=TRUE;
                                    switch (pre_moves_source[i])
                                    {
                                        case PV_MOVE :
                                            pv_moves--;
                                            break;
                                        case HASH_MOVE :
                                            hash_moves--;
                                            break;
                                        case KILLER_MOVE :
                                            killer_moves--;
                                            break;
                                        case SHALLOW_MOVE :
                                            shallow_moves--;
                                            break;
                                        default :
                                            sprintf(message,"Something is wrong... unrecognized source for pre-move.\n");
                                            BOARD_Message();
                                            break;
                                    }
                                }
                            }
                        }
                        if (!already_tried) // If we haven't already tried this move, try it now
                        {
                            steps_change=Check_Extensions(bp,current_move);
                            #ifdef DEBUG_SEARCH
                            {
                                int spaces;
                                
                                if (depth<=DEBUG_DEPTH)
                                {
                                    for (spaces=0; spaces<depth; spaces++) 
                                    {
                                        sprintf(message,"    ");
                                        BOARD_Message();
                                    }
                                    sprintf(message,"Trying move ");
                                    BOARD_Message();
                                    BOARD_Print_Move(current_move);
                                    sprintf(message," (%d steps %d steps_change) at depth %d (%d steps remaining) (%d,%d)\n",current_move->steps,steps_change,depth,steps,lower_bound,upper_bound);
                                    BOARD_Message();
                                }
                            }
                            #endif
                            BOARD_Do_Move(bp,current_move);
//                            #ifdef DEBUG_SEARCH
//                            {
//                                BOARD_Copy_Move(current_move,&move_seq[depth]);
//                            }
//                            #endif
                            depth+=current_move->steps;
                            steps-=current_move->steps*10;
                            steps+=steps_change;
                            if (bp->hashkey!=start_hashkey) // We're not returning to the start position
                            {
                                if (lower_bound==original_lower_bound // which means we don't have a PV move yet,
                                    || steps<=0) // or are at bottom of search-tree, so will Eval() and get exact value anyhow
                                {
                                    if (bp->steps==0)
                                    {
                                        value=-Nega_Max(bp,-upper_bound,-lower_bound,steps);
                                    } else 
                                    {
                                        value=Nega_Max(bp,lower_bound,upper_bound,steps);
                                    }
                                } else
                                {
                                    if (bp->steps==0)
                                    {
//                                        value=-Nega_Max(bp,-upper_bound,-lower_bound,steps);
                                        // Line above is non-nega-scout, lines below are nega-scout
                                        scout_searches++;
                                        value=-Nega_Max(bp,-lower_bound-1,-lower_bound,steps);
                                        if (value>lower_bound) // need to re-search position
                                        {
                                            scout_researches++;
                                            value=-Nega_Max(bp,-upper_bound,-lower_bound,steps);
                                        }
                                    } else 
                                    {
//                                        value=Nega_Max(bp,lower_bound,upper_bound,steps);
                                        // Line above is non-nega-scout, lines below are nega-scout
                                        scout_searches++;
                                        value=Nega_Max(bp,lower_bound,lower_bound+1,steps);                                    
                                        if (value>lower_bound) // need to re-search position
                                        {
                                            scout_researches++;
                                            value=Nega_Max(bp,lower_bound,upper_bound,steps);
                                        }
                                    }
                                }
                                BOARD_Undo_Move(bp,current_move);
                                depth-=current_move->steps;
                                steps+=current_move->steps*10;
                                steps-=steps_change;
                                if (value>lower_bound && !disturbance)
                                {
                                    if (!pre_moves_done)
                                    {
                                        switch (pre_moves_source[i])
                                        {
                                            case PV_MOVE :
                                                pv_moves_good++;
                                                break;
                                            case HASH_MOVE :
                                                hash_moves_good++;
                                                break;
                                            case KILLER_MOVE :
                                                killer_moves_good++;
                                                break;
                                            case SHALLOW_MOVE :
                                                shallow_moves_good++;
                                                break;
                                            default :
                                                sprintf(message,"Something is wrong... unrecognized source for pre-move.\n");
                                                BOARD_Message();
                                                break;
                                        }
                                    }
                                    lower_bound=value;
                                    BOARD_Copy_Move(current_move,&hash_move);
                                    #ifdef DEBUG_SEARCH
                                    {
                                        int spaces;
                                        
                                        if (depth<=DEBUG_DEPTH)
                                        {
                                            for (spaces=0; spaces<depth; spaces++) 
                                            {
                                                sprintf(message,"    ");                                            
                                                BOARD_Message();
                                            }
                                            sprintf(message,"Improved lower bound, new bounds: (%d , %d)\n",lower_bound,upper_bound);
                                            BOARD_Message();
                                        }
                                    }
                                    #endif
                                    if (lower_bound>=upper_bound) // cutoff, so we want to exit right away
                                    {
                                        #ifdef DEBUG_SEARCH
                                        {
                                            int spaces;
                                            
                                            if (depth<=DEBUG_DEPTH)
                                            {
                                                for (spaces=0; spaces<depth; spaces++) 
                                                {
                                                    sprintf(message,"    ");
                                                    BOARD_Message();
                                                }
                                                sprintf(message,"Caused cutoff!\n");
                                                BOARD_Message();
                                            }
                                        }
                                        #endif
                                        if (!pre_moves_done)
                                        {
                                            switch (pre_moves_source[i])
                                            {
                                                case PV_MOVE :
                                                    pv_moves_cutoff++;
                                                    break;
                                                case HASH_MOVE :
                                                    hash_moves_cutoff++;
                                                    break;
                                                case KILLER_MOVE :
                                                    killer_moves_cutoff++;
                                                    break;
                                                case SHALLOW_MOVE :
                                                    shallow_moves_cutoff++;
                                                    break;
                                                default :
                                                    sprintf(message,"Something is wrong... unrecognized source for pre-move.\n");
                                                    BOARD_Message();
                                                    break;
                                            }
                                        }
                                        cutoff=TRUE;
                                        BOARD_Copy_Move(current_move,&killer_move[depth]);
                                        killer_move_set[depth]=TRUE;
                                    } else
                                    {
                                        pv_length[depth]=pv_length[depth+current_move->steps]+current_move->steps;
                                        if (hash_hit[depth+current_move->steps]==-1)
                                        {
                                            hash_hit[depth]=-1;
                                        } else
                                        {
                                            hash_hit[depth]=hash_hit[depth+current_move->steps]+current_move->steps;
                                        }
                                        for (j=depth+current_move->steps; j<depth+pv_length[depth]; j+=pv[depth][j].steps)
                                        {
                                            BOARD_Copy_Move(&pv[depth+current_move->steps][j],&pv[depth][j]);
                                        }
                                        BOARD_Copy_Move(current_move,&pv[depth][depth]);
                                    }
                                    if (depth==0)
                                    {
                                        Show_Search_Info(bp,FALSE,steps,value);
                                    }
                                }
                            } else
                            {
                                BOARD_Undo_Move(bp,current_move);
                                depth-=current_move->steps;
                                steps+=current_move->steps*10;
                                steps-=steps_change;
                            }
                        }
                    }
                }
                if (number_of_moves==0)
                {
                    lower_bound=-500000+bp->move*4+bp->steps;
                    terminal_node=TRUE;
                    pv_length[depth]=0;
                    hash_hit[depth]=-1;
                    if (depth>max_depth) max_depth=depth;
                    #ifdef DEBUG_SEARCH
                    {
                        int spaces;
                        
                        if (depth<=DEBUG_DEPTH)
                        {
                            for (spaces=0; spaces<depth; spaces++) 
                            {
                                sprintf(message,"    ");
                                BOARD_Message();
                            }
                            sprintf(message,"%d is value due to win by immobilization at depth %d.\n",lower_bound,depth);
                            BOARD_Message();
                        }
                    }
                    #endif
                }
            }
        }
        if (!disturbance)
        {
            // Write to hashtable
            if (terminal_node) // Terminal node, so we clearly have an exact value
            {
                #ifdef DEBUG_SEARCH
                {
                    int spaces;
                    
                    if (depth<=DEBUG_DEPTH)
                    {
                        for (spaces=0; spaces<depth; spaces++) 
                        {
                            sprintf(message,"    ");
                            BOARD_Message();
                        }
                        sprintf(message,"Terminal node (exact value %d at depth 0) written to hash table at depth %d.\n",lower_bound,depth);
                        BOARD_Message();
                    }
                }
                #endif
                if (steps>0)
                {
                    HASH_Put_Entry(bp->hashkey,lower_bound,steps,EXACT_VALUE,NULL);
                } else
                {
                    HASH_Put_Entry(bp->hashkey,lower_bound,0,EXACT_VALUE,NULL);
                }
                hash_writes_exact_value++;
            } else
            {
                if (lower_bound==original_lower_bound) // Fail-low, so we have an upper bound for the real value of position
                {
                    #ifdef DEBUG_SEARCH
                    {
                        int spaces;
                        
                        if (depth<=DEBUG_DEPTH)
                        {
                            for (spaces=0; spaces<depth; spaces++) 
                            {
                                sprintf(message,"    ");
                                BOARD_Message();
                            }
                            sprintf(message,"Value (upper-bound value %d at depth %d) written to hash table at depth %d.\n",original_lower_bound,steps,depth);
                            BOARD_Message();
                        }
                    }
                    #endif
                    HASH_Put_Entry(bp->hashkey,original_lower_bound,steps,UPPER_BOUND,NULL);
                    hash_writes_upper_bound++;
                } else if (lower_bound>=original_upper_bound) // Fail-high, so we have a lower bound for the real value of position
                {
                    #ifdef DEBUG_SEARCH
                    {
                        int spaces;
                        
                        if (depth<=DEBUG_DEPTH)
                        {
                            for (spaces=0; spaces<depth; spaces++) 
                            {
                                sprintf(message,"    ");
                                BOARD_Message();
                            }
                            BOARD_Print_Move(&hash_move);
                            sprintf(message," (lower-bound value %d at depth %d) written to hash table at depth %d.\n",original_upper_bound,steps,depth);
                            BOARD_Message();
                        }
                    }
                    #endif
                    HASH_Put_Entry(bp->hashkey,original_upper_bound,steps,LOWER_BOUND,&hash_move);
                    hash_writes_lower_bound++;
                } else // Value is between original lower and upper bounds, so we have an exact value for the position
                {
                    #ifdef DEBUG_SEARCH
                    {
                        int spaces;
                        
                        if (depth<=DEBUG_DEPTH)
                        {
                            for (spaces=0; spaces<depth; spaces++) 
                            {
                                sprintf(message,"    ");
                                BOARD_Message();
                            }
                            BOARD_Print_Move(&hash_move);
                            sprintf(message," (exact value %d at depth %d) written to hash table at depth %d.\n",lower_bound,steps,depth);
                            BOARD_Message();
                        }
                    }
                    #endif
                    HASH_Put_Entry(bp->hashkey,lower_bound,steps,EXACT_VALUE,&hash_move);
                    hash_writes_exact_value++;
                }
            }
        }
    }
    return lower_bound;
}

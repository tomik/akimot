#include <stdlib.h>
#include <stdio.h>

#include "board.h"
#include "eval.h"

#define ELEPHANT_VALUE 20000
#define CAMEL_VALUE 5000
#define HORSE_VALUE 3000
#define DOG_VALUE 1800
#define CAT_VALUE 1500
#define RABBIT_VALUE 1000

#define RABBIT_FREE_AHEAD 1000
#define RABBIT_FRIENDLY_AHEAD 500
#define RABBIT_FREE_SIDE 300
#define RABBIT_FRIENDLY_SIDE 200

// board constants
static const int trap_square[4]={33,36,63,66};
//static const int direction[4]={NORTH,EAST,SOUTH,WEST};

static const int adjacent_trap[100]={0,0,0,0,0,0,0,0,0,0,0,

                                      0, 0, 0, 0, 0, 0, 0, 0,     0,0,                               
                                      0, 0,33, 0, 0,36, 0, 0,     0,0,                               
                                      0,33, 0,33,36, 0,36, 0,     0,0,                               
                                      0, 0,33, 0, 0,36, 0, 0,     0,0,                               
                                      0, 0,63, 0, 0,66, 0, 0,     0,0,                               
                                      0,63, 0,63,66, 0,66, 0,     0,0,                               
                                      0, 0,63, 0, 0,66, 0, 0,     0,0,                               
                                      0, 0, 0, 0, 0, 0, 0, 0,     0,0,                               

                                     0,0,0,0,0,0,0,0,0};

static const int adjacent2_trap[100]={0,0,0,0,0,0,0,0,0,0,0,

                                       0, 0,33, 0, 0,36, 0, 0,     0,0,                               
                                       0,33, 0,33,36, 0,36, 0,     0,0,                               
                                      33, 0, 0,36,33, 0, 0,36,     0,0,                               
                                       0,33,63,33,36,66,36, 0,     0,0,                               
                                       0,63,33,63,66,36,66, 0,     0,0,                               
                                      63, 0, 0,66,63, 0, 0,66,     0,0,                               
                                       0,63, 0,63,66, 0,66, 0,     0,0,                               
                                       0, 0,63, 0, 0,66, 0, 0,     0,0,                               

                                      0,0,0,0,0,0,0,0,0};
                                                                    
int EVAL_Eval(board_t *bp, int verbose)

// Evaluation is done from gold's perspective.  At the end of the evaluation, it's adjusted to be seen from current player's perspective.

{
    // evaluation constants
    static const int piece_value[7]={0,RABBIT_VALUE,CAT_VALUE,DOG_VALUE,HORSE_VALUE,CAMEL_VALUE,ELEPHANT_VALUE};
    // variables        
    
    // utility variables
    int side_mask[OWNER_MASK];
     
    // loop variables
    int square; 
    int side;
    int trap;
    int dir;
    int i;
            
    // value variables
    int value=0;
    int material_value[2]={0,0};
    int trap_value[2]={0,0};
    int rabbit_value[2]={0,0};
            
    // how many of the pieces do the players have, and where are they?
    int elephants[2]={0,0};
    int elephant_pos[2][1];
    int camels[2]={0,0};
    int camel_pos[2][1];
    int horses[2]={0,0};
    int horse_pos[2][2];
    int dogs[2]={0,0};
    int dog_pos[2][2];
    int cats[2]={0,0};
    int cat_pos[2][2];
    int rabbits[2]={0,0};
    int rabbit_pos[2][8];
    
    // trap evaluation variables
    int trap_adjacent[2];
    int trap_adjacent_strength[2];
    int trap_adjacent_strongest[2];
                                     
    // material evaluation variables
    int material[100]; // What is the piece on this square worth?
    int piece_frozen;
    int piece_adjacent_stronger_enemy;
    int piece_adjacent_empty;
    int piece_adjacent_strongest[2];
    int piece_adjacent[2];
    int piece_adjacent_trap;
    
    // rabbit evaluation variables
    int row;
    
    // Initialize some evaluation stuff

    side_mask[GOLD]=0;
    side_mask[SILVER]=1;

    // Determine extra information about the board state

    for (square=11; square<=88; square++) // loop over board, initialize board state info and find where all the pieces are.
    {
        if (square%10==9) square+=2;
        switch (PIECE(square))
        {
            case ELEPHANT_PIECE :
                elephant_pos[side_mask[OWNER(square)]][elephants[side_mask[OWNER(square)]]]=square;
                elephants[side_mask[OWNER(square)]]++;
                break;
            case CAMEL_PIECE :
                camel_pos[side_mask[OWNER(square)]][camels[side_mask[OWNER(square)]]]=square;
                camels[side_mask[OWNER(square)]]++;
                break;
            case HORSE_PIECE :
                horse_pos[side_mask[OWNER(square)]][horses[side_mask[OWNER(square)]]]=square;
                horses[side_mask[OWNER(square)]]++;
                break;
            case DOG_PIECE :
                dog_pos[side_mask[OWNER(square)]][dogs[side_mask[OWNER(square)]]]=square;
                dogs[side_mask[OWNER(square)]]++;
                break;
            case CAT_PIECE :
                cat_pos[side_mask[OWNER(square)]][cats[side_mask[OWNER(square)]]]=square;
                cats[side_mask[OWNER(square)]]++;
                break;
            case RABBIT_PIECE :
                rabbit_pos[side_mask[OWNER(square)]][rabbits[side_mask[OWNER(square)]]]=square;
                rabbits[side_mask[OWNER(square)]]++;
                break;
        }
        if (OWNER(square)==GOLD || OWNER(square)==SILVER)
        {
            material[square]=piece_value[PIECE(square)];
        } else
        {
            material[square]=0;
        }
    }
    
    // Evaluate trap squares, decide trap ownership.

    if (verbose)
    {
        sprintf(message,"Evaluating traps:\n");
        BOARD_Message();
    }       
    for (trap=0; trap<4; trap++)
    {
        for (side=0; side<2; side++)
        {
            trap_adjacent[side]=0;
            trap_adjacent_strength[side]=0;
            trap_adjacent_strongest[side]=0;
        }
        for (dir=0; dir<4; dir++)
        {
            switch (OWNER(trap_square[trap]+direction[dir]))
            {
                case GOLD :
                    trap_adjacent[0]++;
                    trap_adjacent_strength[0]+=PIECE(trap_square[trap]+direction[dir]);
                    if (PIECE(trap_square[trap]+direction[dir])>trap_adjacent_strongest[0])
                    {
                        trap_adjacent_strongest[0]=PIECE(trap_square[trap]+direction[dir]);
                    }
                    break;
                case SILVER :
                    trap_adjacent[1]++;
                    trap_adjacent_strength[1]+=PIECE(trap_square[trap]+direction[dir]);
                    if (PIECE(trap_square[trap]+direction[dir])>trap_adjacent_strongest[1])
                    {
                        trap_adjacent_strongest[1]=PIECE(trap_square[trap]+direction[dir]);
                    }
                    break;
            }
        }
        // Basically, 200 points are given out per trap.  50 to whoever has the strongest piece by the trap, 
        // and 150 points split according to total strength of pieces, with two neutral strength added.
        
        // case 1 - only one side has pieces by the trap.
        if (trap_adjacent[0]>0 && trap_adjacent[1]==0) 
        {
            trap_value[0]+=50+trap_adjacent_strength[0]*150/(trap_adjacent_strength[0]+1);
            if (verbose)
            {
                PRINT_SQUARE(trap_square[trap]);
                sprintf(message," is worth Gold (%d) - Silver (%d).\n",50+trap_adjacent_strength[0]*150/(trap_adjacent_strength[0]+1),0);
                BOARD_Message();
            }
        }
        if (trap_adjacent[1]>0 && trap_adjacent[0]==0)
        {
            trap_value[1]+=50+trap_adjacent_strength[1]*150/(trap_adjacent_strength[1]+1);
            if (verbose)
            {
                PRINT_SQUARE(trap_square[trap]);
                sprintf(message," is worth Gold (%d) - Silver (%d).\n",0,50+trap_adjacent_strength[1]*150/(trap_adjacent_strength[1]+1));
                BOARD_Message();
            }
        }
        // case 2 - both sides have pieces by the trap.
        if (trap_adjacent[0]>0 && trap_adjacent[1]>0)
        {
            // subcase 1 - they are equally strong.  Split 100 points according to number of pieces.
            if (trap_adjacent_strongest[0]==trap_adjacent_strongest[1])
            {
                trap_value[0]+=trap_adjacent_strength[0]*200/(trap_adjacent_strength[0]+trap_adjacent_strength[1]+1);
                trap_value[1]+=trap_adjacent_strength[1]*200/(trap_adjacent_strength[0]+trap_adjacent_strength[1]+1);
                if (verbose)
                {
                    PRINT_SQUARE(trap_square[trap]);
                    sprintf(message," is worth Gold (%d) - Silver (%d).\n",trap_adjacent_strength[0]*200/(trap_adjacent_strength[0]+trap_adjacent_strength[1]+1),trap_adjacent_strength[1]*200/(trap_adjacent_strength[0]+trap_adjacent_strength[1]+1));
                    BOARD_Message();
                }
            }
            // subcase 2 - gold is stronger.  Give 50 points to gold, and split 50 according to number of pieces.
            if (trap_adjacent_strongest[0]>trap_adjacent_strongest[1])
            {
                trap_value[0]+=50+trap_adjacent_strength[0]*150/(trap_adjacent_strength[0]+trap_adjacent_strength[1]+1);
                trap_value[1]+=trap_adjacent_strength[1]*150/(trap_adjacent_strength[0]+trap_adjacent_strength[1]+1);
                if (verbose)
                {
                    PRINT_SQUARE(trap_square[trap]);
                    sprintf(message," is worth Gold (%d) - Silver (%d).\n",50+trap_adjacent_strength[0]*150/(trap_adjacent_strength[0]+trap_adjacent_strength[1]+1),trap_adjacent_strength[1]*150/(trap_adjacent_strength[0]+trap_adjacent_strength[1]+1));
                    BOARD_Message();
                }
            }
            // subcase 3 - silver is stronger.  Give 50 points to silver, and split 50 according to number of pieces.
            if (trap_adjacent_strongest[1]>trap_adjacent_strongest[0])
            {
                trap_value[0]+=trap_adjacent_strength[0]*150/(trap_adjacent_strength[0]+trap_adjacent_strength[1]+1);
                trap_value[1]+=50+trap_adjacent_strength[1]*150/(trap_adjacent_strength[0]+trap_adjacent_strength[1]+1);
                if (verbose)
                {
                    PRINT_SQUARE(trap_square[trap]);
                    sprintf(message," is worth Gold (%d) - Silver (%d).\n",trap_adjacent_strength[0]*150/(trap_adjacent_strength[0]+trap_adjacent_strength[1]+1),50+trap_adjacent_strength[1]*150/(trap_adjacent_strength[0]+trap_adjacent_strength[1]+1));
                    BOARD_Message();
                }
            }
        }
        // special case - give minus for (possible) frames
        if (OWNER(trap_square[trap])==GOLD && trap_adjacent[1]>2)
        {
            material[trap_square[trap]]=material[trap_square[trap]]*4/5; // Trapped piece loses 20% of its value
            if (verbose)
            {
                sprintf(message,"Piece at ");
                BOARD_Message();
                PRINT_SQUARE(trap_square[trap]);
                sprintf(message," is worth %d due to being (possibly) framed.\n",material[trap_square[trap]]);
                BOARD_Message();
            }
        }
        if (OWNER(trap_square[trap])==SILVER && trap_adjacent[0]>2)
        {
            material[trap_square[trap]]=material[trap_square[trap]]*4/5; // Trapped piece loses 20% of its value
            if (verbose)
            {
                sprintf(message,"Piece at ");
                BOARD_Message();
                PRINT_SQUARE(trap_square[trap]);
                sprintf(message," is worth %d due to being (possibly) framed.\n",material[trap_square[trap]]);
                BOARD_Message();
            }
        }
    }
    
    // Evaluate material and individual pieces.

    for (side=0; side<2; side++)
    {
        for (i=0; i<cats[side]; i++)
        {
            switch (side)
            {
                case 0 : 
                    row=ROW(cat_pos[0][i]);
                    break;
                case 1 :
                    row=9-ROW(cat_pos[1][i]);
                    break;
            }
            if (row>3)
            {
                material[cat_pos[side][i]]=material[cat_pos[side][i]]*197/200; // Advanced cat lose 1.5 % of its value
                if (verbose)
                {
                    PRINT_SQUARE(cat_pos[side][i]);
                    sprintf(message," is worth %d due to being an advanced cat.\n",material[cat_pos[side][i]]);
                    BOARD_Message();
                }
            } else if (row==3)
            {
                material[cat_pos[side][i]]=material[cat_pos[side][i]]*199/200; // Slightly advanced cat lose 0.5 % of its value
                if (verbose)
                {
                    PRINT_SQUARE(cat_pos[side][i]);
                    sprintf(message," is worth %d due to being a slightly advanced cat.\n",material[cat_pos[side][i]]);
                    BOARD_Message();
                }
            }
        }
    
        for (i=0; i<dogs[side]; i++)
        {
            switch (side)
            {
                case 0 : 
                    row=ROW(dog_pos[0][i]);
                    break;
                case 1 :
                    row=9-ROW(dog_pos[1][i]);
                    break;
            }
            if (row>3)
            {
                material[dog_pos[side][i]]=material[dog_pos[side][i]]*197/200; // Advanced cat lose 1.5 % of its value
                if (verbose)
                {
                    PRINT_SQUARE(dog_pos[side][i]);
                    sprintf(message," is worth %d due to being an advanced dog.\n",material[dog_pos[side][i]]);
                    BOARD_Message();
                }
            } else if (row==3)
            {
                material[dog_pos[side][i]]=material[dog_pos[side][i]]*199/200; // Slightly advanced cat lose 0.5 % of its value
                if (verbose)
                {
                    PRINT_SQUARE(dog_pos[side][i]);
                    sprintf(message," is worth %d due to being a slightly advanced dog.\n",material[dog_pos[side][i]]);
                    BOARD_Message();
                }
            }
        }
    }

    for (square=11; square<=88; square++)
    {    
        if (square%10==9) square+=2;
        if (OWNER(square)==GOLD || OWNER(square)==SILVER)
        {
            // Check if it's frozen, number of adjacent empty, strongest adjacent, and all that
            piece_adjacent[0]=0;
            piece_adjacent[1]=0;
            piece_adjacent_empty=0;
            piece_adjacent_strongest[0]=0;
            piece_adjacent_strongest[1]=0;
            for (dir=0; dir<4; dir++)
            {
                switch (OWNER(square+direction[dir]))
                {
                    case GOLD :
                        piece_adjacent[0]++;
                        if (PIECE(square+direction[dir])>piece_adjacent_strongest[0])
                        {
                            piece_adjacent_strongest[0]=PIECE(square+direction[dir]);
                        }
                        break;
                    case SILVER :
                        piece_adjacent[1]++;
                        if (PIECE(square+direction[dir])>piece_adjacent_strongest[1])
                        {
                            piece_adjacent_strongest[1]=PIECE(square+direction[dir]);
                        }
                        break;
                    case EMPTY :
                        piece_adjacent_empty++;
                        break;
                }
            }
            switch (OWNER(square))
            {
                case GOLD :
                    piece_adjacent_stronger_enemy=piece_adjacent_strongest[1]>PIECE(square);
                    piece_frozen=piece_adjacent_stronger_enemy && piece_adjacent[0]==0;
                    break;
                case SILVER :
                    piece_adjacent_stronger_enemy=piece_adjacent_strongest[0]>PIECE(square);
                    piece_frozen=piece_adjacent_stronger_enemy && piece_adjacent[1]==0;
                    break;
            }
            if (piece_frozen)
            {
                material[square]=material[square]*9/10; // Frozen piece loses 10% of its value
                if (verbose)
                {
                    PRINT_SQUARE(square);
                    sprintf(message," is worth %d due to being frozen.\n",material[square]);
                    BOARD_Message();
                }
            }
            if (piece_adjacent_empty==0) 
            {
                material[square]=material[square]*199/200; // Immobile piece loses 0.5% of its value
                if (verbose)
                {
                    PRINT_SQUARE(square);
                    sprintf(message," is worth %d due to being immobile.\n",material[square]);
                    BOARD_Message();
                }
            }
            if ((piece_frozen || piece_adjacent_empty==0) && piece_adjacent_stronger_enemy) // Our piece has limited mobility, and there is a stronger enemy piece adjacent
            {
                // Check if it's held hostage or threatened by a capture
                if (adjacent_trap[square]) // It's adjacent to a trap
                {
                    // If we have no other piece next to the trap, then consider this piece to be threatened, losing 30% of its value
                    piece_adjacent_trap=0;
                    for (dir=0; dir<4; dir++)
                    {
                        if (OWNER(adjacent_trap[square]+direction[dir])==OWNER(square))
                        {
                            piece_adjacent_trap++;
                        }
                    }
                    if (piece_adjacent_trap==1)
                    {
                        material[square]=material[square]*7/10;
                        if (verbose)
                        {
                            PRINT_SQUARE(square);
                            sprintf(message," is worth %d due to being threatened (distance 1).\n",material[square]);
                            BOARD_Message();
                        }
                    }
                }
                if (adjacent2_trap[square] && BOARD(adjacent2_trap[square])==EMPTY_SQUARE) 
                // It's two steps away from an empty trap
                {
                    // If we have no piece next to the trap,
                    // Really - should check so that there is a free path to trap.
                    // then consider this piece to be threatened, losing 30% of its value
                    piece_adjacent_trap=0;
                    for (dir=0; dir<4; dir++)
                    {
                        if (OWNER(adjacent2_trap[square]+direction[dir])==OWNER(square))
                        {
                            piece_adjacent_trap++;
                        }
                    }
                    if (piece_adjacent_trap==0)
                    {
                        material[square]=material[square]*7/10;
                        if (verbose)
                        {
                            PRINT_SQUARE(square);
                            sprintf(message," is worth %d due to being threatened (distance 2).\n",material[square]);
                            BOARD_Message();
                        }
                    }
                }
            }
            // Another case - if adjacent to a trap, and no other friendly piece adjacent, various possibilities for being threatened....
            switch (OWNER(square))
            {
                case GOLD :
                    material_value[0]+=material[square];
                    break;
                case SILVER :
                    material_value[1]+=material[square];
                    break;
            }
        }
    }
    
    // Evaluate rabbits

    for (i=0; i<rabbits[0]; i++)
    {
        row=ROW(rabbit_pos[0][i]);
        rabbit_value[0]+=(row-1)*(row-1)*(row-1);
        if (row==7)
        {
            switch (OWNER(rabbit_pos[0][i]+NORTH))
            {
                case EMPTY :
                    rabbit_value[0]+=RABBIT_FREE_AHEAD;
                    if (verbose)
                    {
                        PRINT_SQUARE(rabbit_pos[0][i]);
                        sprintf(message," : gold rabbit value increased by %d due to free space ahead\n",RABBIT_FREE_AHEAD);
                        BOARD_Message();
                    }
                    break;
                case GOLD :
                    rabbit_value[0]+=RABBIT_FRIENDLY_AHEAD;
                    if (verbose)
                    {
                        PRINT_SQUARE(rabbit_pos[0][i]);
                        sprintf(message," : gold rabbit value increased by %d due to friendly piece ahead\n",RABBIT_FRIENDLY_AHEAD);
                        BOARD_Message();
                    }
                    break;
            }
            switch (OWNER(rabbit_pos[0][i]+EAST))
            {
                case EMPTY :
                    rabbit_value[0]+=RABBIT_FREE_SIDE;
                    if (verbose)
                    {
                        PRINT_SQUARE(rabbit_pos[0][i]);
                        sprintf(message," : gold rabbit value increased by %d due to free space to the east\n",RABBIT_FREE_SIDE);
                        BOARD_Message();
                    }
                    break;
                case GOLD :
                    rabbit_value[0]+=RABBIT_FRIENDLY_SIDE;
                    if (verbose)
                    {
                        PRINT_SQUARE(rabbit_pos[0][i]);
                        sprintf(message," : gold rabbit value increased by %d due to friendly piece to the east\n",RABBIT_FRIENDLY_SIDE);
                        BOARD_Message();
                    }
                    break;
            }
            switch (OWNER(rabbit_pos[0][i]+WEST))
            {
                case EMPTY :
                    rabbit_value[0]+=RABBIT_FREE_SIDE;
                    if (verbose)
                    {
                        PRINT_SQUARE(rabbit_pos[0][i]);
                        sprintf(message," : gold rabbit value increased by %d due to free space to the west\n",RABBIT_FREE_SIDE);
                        BOARD_Message();
                    }
                    break;
                case GOLD :
                    rabbit_value[0]+=RABBIT_FRIENDLY_SIDE;
                    if (verbose)
                    {
                        PRINT_SQUARE(rabbit_pos[0][i]);
                        sprintf(message," : gold rabbit value increased by %d due to friendly piece to the west\n",RABBIT_FRIENDLY_SIDE);
                        BOARD_Message();
                    }
                    break;
            }
        }
    }
    for (i=0; i<rabbits[1]; i++)
    {
        row=9-ROW(rabbit_pos[1][i]);
        rabbit_value[1]+=(row-1)*(row-1);
        if (row==7)
        {
            switch (OWNER(rabbit_pos[1][i]+SOUTH))
            {
                case EMPTY :
                    rabbit_value[1]+=RABBIT_FREE_AHEAD;
                    if (verbose)
                    {
                        PRINT_SQUARE(rabbit_pos[1][i]);
                        sprintf(message," : silver rabbit value increased by %d due to free space ahead\n",RABBIT_FREE_AHEAD);
                        BOARD_Message();
                    }
                    break;
                case SILVER :
                    rabbit_value[1]+=RABBIT_FRIENDLY_AHEAD;
                    if (verbose)
                    {
                        PRINT_SQUARE(rabbit_pos[1][i]);
                        sprintf(message," : silver rabbit value increased by %d due to friendly piece ahead\n",RABBIT_FRIENDLY_AHEAD);
                        BOARD_Message();
                    }
                    break;
            }
            switch (OWNER(rabbit_pos[1][i]+EAST))
            {
                case EMPTY :
                    rabbit_value[1]+=RABBIT_FREE_SIDE;
                    if (verbose)
                    {
                        PRINT_SQUARE(rabbit_pos[1][i]);
                        sprintf(message," : silver rabbit value increased by %d due to free space to the east\n",RABBIT_FREE_SIDE);
                        BOARD_Message();
                    }
                    break;
                case SILVER :
                    rabbit_value[1]+=RABBIT_FRIENDLY_SIDE;
                    if (verbose)
                    {
                        PRINT_SQUARE(rabbit_pos[1][i]);
                        sprintf(message," : silver rabbit value increased by %d due to friendly piece to the east\n",RABBIT_FRIENDLY_SIDE);
                        BOARD_Message();
                    }
                    break;
            }
            switch (OWNER(rabbit_pos[1][i]+WEST))
            {
                case EMPTY :
                    rabbit_value[1]+=RABBIT_FREE_SIDE;
                    if (verbose)
                    {
                        PRINT_SQUARE(rabbit_pos[1][i]);
                        sprintf(message," : silver rabbit value increased by %d due to free space to the west\n",RABBIT_FREE_SIDE);
                        BOARD_Message();
                    }
                    break;
                case SILVER :
                    rabbit_value[1]+=RABBIT_FRIENDLY_SIDE;
                    if (verbose)
                    {
                        PRINT_SQUARE(rabbit_pos[1][i]);
                        sprintf(message," : silver rabbit value increased by %d due to friendly piece to the west\n",RABBIT_FRIENDLY_SIDE);
                        BOARD_Message();
                    }
                    break;
            }
        }
    }
    
    // Add up all the factors
           
    value+=material_value[0]-material_value[1];
    value+=trap_value[0]-trap_value[1];
    value+=rabbit_value[0]-rabbit_value[1];

    if (verbose)
    {
        sprintf(message,"Material value Gold (%d) - Silver (%d)\n",material_value[0],material_value[1]);
        BOARD_Message();
        sprintf(message,"Trap value Gold (%d) - Silver (%d)\n",trap_value[0],trap_value[1]);
        BOARD_Message();
        sprintf(message,"Rabbit value Gold (%d) - Silver (%d)\n",rabbit_value[0],rabbit_value[1]);
        BOARD_Message();
    }
        
    // Adjust evaluation to be from the perspective of the present player.

    if (bp->at_move==SILVER)
    {
        value=-value;
    }
    return value;
}

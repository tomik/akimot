#include <stdio.h>

#include "board.h"
#include "hash.h"

const int direction[4]={NORTH,EAST,SOUTH,WEST};
const int trap[4]={33,36,63,66};
char message[1000];

static FILE *fp;

void BOARD_Init(board_t *bp)
{
    int i, j;
    
    for (i=0; i<100; i++)
    {
        BOARD(i)=OFF_BOARD_SQUARE;
    }
    for (i=1; i<9; i++)
    {
        for (j=1; j<9; j++)
        {
            BOARD(10*i+j)=EMPTY_SQUARE;
        }
    }
    bp->steps=0;
    bp->at_move=GOLD;
    BOARD_Calculate_Hashkey(bp);
}

void BOARD_Copy_Move(move_t *mpfrom, move_t *mpto)
{
    int i;
    
    mpto->steps=mpfrom->steps;
    mpto->pass=mpfrom->pass;
    mpto->hashkey=mpfrom->hashkey;
    for (i=0; i<2; i++)
    {
        mpto->piece[i]=mpfrom->piece[i];
        mpto->from[i]=mpfrom->from[i];
        mpto->to[i]=mpfrom->to[i];
        mpto->capture_square[i]=mpfrom->capture_square[i];
        mpto->captured_piece[i]=mpfrom->captured_piece[i];
    }
}

void BOARD_Calculate_Hashkey(board_t *bp)
{
    unsigned long long int hashkey=0;
    int i, j;
    
    for (i=1; i<9; i++)
    {
        for (j=1; j<9; j++)
        {
            hashkey^=hashkey_piece[i*10+j][BOARD(i*10+j)];
        }
    }
    hashkey^=hashkey_step[bp->steps];
    hashkey^=hashkey_at_move[bp->at_move];
    bp->hashkey=hashkey;
}

void BOARD_Do_Move(board_t *bp, move_t *mp)
{
    int i;
    
    bp->hashkey^=hashkey_step[bp->steps];
    if (mp->pass) // pass move
    {
        bp->at_move^=FLIP_SIDE;
        bp->hashkey^=hashkey_at_move[GOLD]^hashkey_at_move[SILVER];
        bp->steps=0;
        bp->move++;
    } else 
    {
        for (i=0; i<mp->steps; i++) // move the pieces
        {
            BOARD(mp->to[i])=mp->piece[i];
            BOARD(mp->from[i])=EMPTY_SQUARE;
            bp->hashkey^=hashkey_piece[mp->from[i]][mp->piece[i]]^hashkey_piece[mp->to[i]][mp->piece[i]];
            if (mp->capture_square[i]) // remove piece due to capture
            {
                BOARD(mp->capture_square[i])=EMPTY_SQUARE;
                bp->hashkey^=hashkey_piece[mp->capture_square[i]][mp->captured_piece[i]];
            }
        }
        bp->steps+=mp->steps; // update how many steps have been done
        if (bp->steps==4) // if 4 steps have been done, change to other player's turn
        {
            bp->at_move^=FLIP_SIDE;
            bp->hashkey^=hashkey_at_move[GOLD]^hashkey_at_move[SILVER];
            bp->steps=0;
            bp->move++;
        }
    }
    bp->hashkey^=hashkey_step[bp->steps];
}

void BOARD_Undo_Move(board_t *bp, move_t *mp)
{
    int i;
    
    bp->hashkey^=hashkey_step[bp->steps];
    if (mp->pass) // pass move
    {
        bp->at_move^=FLIP_SIDE;
        bp->hashkey^=hashkey_at_move[GOLD]^hashkey_at_move[SILVER];
        bp->steps=4-mp->steps;
        bp->move--;
    } else 
    {
        if (bp->steps==0) // if at start of turn, then the move we want to undo was by the other player
        {
            bp->at_move^=FLIP_SIDE;
            bp->hashkey^=hashkey_at_move[GOLD]^hashkey_at_move[SILVER];
            bp->steps=4;
            bp->move--;
        }
        bp->steps-=mp->steps; // update how many steps have been done
        for (i=mp->steps-1; i>=0; i--)
        {
            if (mp->capture_square[i]) // put back any captured piece
            {
                BOARD(mp->capture_square[i])=mp->captured_piece[i];
                bp->hashkey^=hashkey_piece[mp->capture_square[i]][mp->captured_piece[i]];
            }
            BOARD(mp->from[i])=mp->piece[i]; // move the piece back
            BOARD(mp->to[i])=EMPTY_SQUARE;
            bp->hashkey^=hashkey_piece[mp->from[i]][mp->piece[i]]^hashkey_piece[mp->to[i]][mp->piece[i]];
        }
    }
    bp->hashkey^=hashkey_step[bp->steps];
}

int BOARD_Read_Position(board_t *bp, char *file_name) // returns 0 if file opened and read successfully
{
    FILE *filep;
    char line[100];
    int error_code=0;
    int move=0;
    char side;
    int i, j;
    
    filep=fopen(file_name,"r");
    if (filep==NULL)
    {
        error_code=1;
    }
    if (!error_code)
    {
        fgets(line,100,filep); // line with move number and side to move
        for (i=0; line[i]>='0' && line[i]<='9'; i++)
        {
            move=move*10+line[i]-'0';
        }
        bp->move=2*move-2;
        if (line[i]=='w')
        {
            bp->at_move=GOLD;
        } else 
        {
            bp->at_move=SILVER;
            bp->move++;
        }
        fgets(line,100,filep); // line with top border of board
        for (i=1; i<9; i++) // do this for each of the 8 lines of the board
        {
            fgets(line,100,filep);
            for (j=1; j<9; j++)
            {
                switch(line[2*j+1])
                {
                    case 'E' :
                        BOARD(i*10+j)=(GOLD | ELEPHANT_PIECE);
                        break;
                    case 'M' :
                        BOARD(i*10+j)=(GOLD | CAMEL_PIECE);
                        break;
                    case 'H' :
                        BOARD(i*10+j)=(GOLD | HORSE_PIECE);
                        break;
                    case 'D' :
                        BOARD(i*10+j)=(GOLD | DOG_PIECE);
                        break;
                    case 'C' :
                        BOARD(i*10+j)=(GOLD | CAT_PIECE);
                        break;
                    case 'R' :
                        BOARD(i*10+j)=(GOLD | RABBIT_PIECE);
                        break;
                    case 'e' :
                        BOARD(i*10+j)=(SILVER | ELEPHANT_PIECE);
                        break;
                    case 'm' :
                        BOARD(i*10+j)=(SILVER | CAMEL_PIECE);
                        break;
                    case 'h' :
                        BOARD(i*10+j)=(SILVER | HORSE_PIECE);
                        break;
                    case 'd' :
                        BOARD(i*10+j)=(SILVER | DOG_PIECE);
                        break;
                    case 'c' :
                        BOARD(i*10+j)=(SILVER | CAT_PIECE);
                        break;
                    case 'r' :
                        BOARD(i*10+j)=(SILVER | RABBIT_PIECE);
                        break;
                    case ' ' : case 'X' :
                        BOARD(i*10+j)=EMPTY_SQUARE;
                        break;
                    default :
                        sprintf(message,"Unknown character encountered while reading board.\n");
                        BOARD_Message();
                        error_code=1;
                        break;
                }
            }
        }
    }
    fclose(filep);
    if (!error_code)
    {
        BOARD_Calculate_Hashkey(bp);
    }
    return error_code;
}

void BOARD_Print_Position(board_t *bp)
{
    int i, j;
    
    sprintf(message,"Move %d,  Step %d,  ",bp->move/2+1,bp->steps);
    BOARD_Message();
    if (bp->at_move==GOLD)
    {
        sprintf(message,"Gold at move.\n");
        BOARD_Message();
    } else if (bp->at_move==SILVER)
    {
        sprintf(message,"Silver at move.\n");
        BOARD_Message();
    } else
    {
        sprintf(message,"?????? at move.\n");
        BOARD_Message();
    }
    sprintf(message," +-----------------+\n");
    BOARD_Message();
    for (i=1; i<9; i++)
    {
        sprintf(message,"%d| ",9-i);
        BOARD_Message();
        for (j=1; j<9; j++)
        {
            switch(BOARD(i*10+j))
            {
                case (GOLD | ELEPHANT_PIECE) :
                    sprintf(message,"E ");
                    BOARD_Message();
                    break;
                case (GOLD | CAMEL_PIECE) :
                    sprintf(message,"M ");
                    BOARD_Message();
                    break;
                case (GOLD | HORSE_PIECE) :
                    sprintf(message,"H ");
                    BOARD_Message();
                    break;
                case (GOLD | DOG_PIECE) :
                    sprintf(message,"D ");
                    BOARD_Message();
                    break;
                case (GOLD | CAT_PIECE) :
                    sprintf(message,"C ");
                    BOARD_Message();
                    break;
                case (GOLD | RABBIT_PIECE) :
                    sprintf(message,"R ");
                    BOARD_Message();
                    break;
                case (SILVER | ELEPHANT_PIECE) :
                    sprintf(message,"e ");
                    BOARD_Message();
                    break;
                case (SILVER | CAMEL_PIECE) :
                    sprintf(message,"m ");
                    BOARD_Message();
                    break;
                case (SILVER | HORSE_PIECE) :
                    sprintf(message,"h ");
                    BOARD_Message();
                    break;
                case (SILVER | DOG_PIECE) :
                    sprintf(message,"d ");
                    BOARD_Message();
                    break;
                case (SILVER | CAT_PIECE) :
                    sprintf(message,"c ");
                    BOARD_Message();
                    break;
                case (SILVER | RABBIT_PIECE) :
                    sprintf(message,"r ");
                    BOARD_Message();
                    break;
                case EMPTY_SQUARE :
                    if ((i==3 || i==6) && (j==3 || j==6))
                    {
                        sprintf(message,"- ");
                        BOARD_Message();
                    } else
                    {
                        sprintf(message,". ");
                        BOARD_Message();
                    }
                    break;
                default :
                    sprintf(message,"? ");
                    BOARD_Message();
                    break;
            }
        }
        sprintf(message,"|\n");
        BOARD_Message();
    }
    sprintf(message," +-----------------+\n");
    BOARD_Message();
    sprintf(message,"   a b c d e f g h\n");
    BOARD_Message();
    sprintf(message,"Hashkey: %#.8X%.8X\n",(unsigned long int)(bp->hashkey>>32),(unsigned long int)(bp->hashkey&0xFFFFFFFFULL));
    BOARD_Message();
}

int BOARD_Generate_Moves(board_t *bp, move_t ml[MAX_NUMBER_MOVES])
{
    int i, j, k;
    int square, trap_index;
    int adjacent_friend, adjacent_stronger_enemy, capture;
    int number_of_moves=0;
    
    for (square=11; square<89; square++)
    {
        if (OWNER(square)==bp->at_move) // one of side to move's pieces found.
        {
            adjacent_friend=FALSE;
            adjacent_stronger_enemy=FALSE;
            for (i=0; i<4; i++) // check if piece is frozen
            {
                if (OWNER(square+direction[i])==bp->at_move) // is a friendly piece adjacent?
                {
                    adjacent_friend=TRUE;
                } else if (OWNER(square+direction[i])==(bp->at_move^FLIP_SIDE) // is an enemy piece adjacent
                    && PIECE(square+direction[i])>PIECE(square)) // and stronger than this piece?
                {
                    adjacent_stronger_enemy=TRUE;
                }
            }
            if (adjacent_friend || !adjacent_stronger_enemy) // piece not frozen, so we can generate moves for it
            {
                if (bp->steps<3) // generate double moves, only if enough steps left in turn for them
                {
                    for (i=0; i<4; i++) // for each direction, check if weaker enemy neighbour is there
                    {
                        if (OWNER(square+direction[i])==(bp->at_move^FLIP_SIDE) // is an enemy piece adjacent
                            && PIECE(square+direction[i])<PIECE(square)) // and weaker
                        {
                            for (j=0; j<4; j++) // check if we can pull it by moving in one of the four directions
                            {
                                if (BOARD(square+direction[j])==EMPTY_SQUARE) // empty square found, so we can move there pulling the piece behind us
                                {
                                    ml[number_of_moves].steps=2;
                                    ml[number_of_moves].pass=FALSE;
                                    ml[number_of_moves].piece[0]=BOARD(square);
                                    ml[number_of_moves].from[0]=square;
                                    ml[number_of_moves].to[0]=square+direction[j];
                                    ml[number_of_moves].piece[1]=BOARD(square+direction[i]);
                                    ml[number_of_moves].from[1]=square+direction[i];
                                    ml[number_of_moves].to[1]=square;
                                    ml[number_of_moves].hashkey=hashkey_piece[square][BOARD(square)]^hashkey_piece[square+direction[j]][BOARD(square)]
                                        ^hashkey_piece[square+direction[i]][BOARD(square+direction[i])]^hashkey_piece[square][BOARD(square+direction[i])];
                                    BOARD(square+direction[j])=BOARD(square); // temporarily move the first piece, to check for captures
                                    BOARD(square)=EMPTY_SQUARE;
                                    // check trap squares for capture
                                    ml[number_of_moves].capture_square[0]=0;
                                    ml[number_of_moves].capture_square[1]=0;
                                    for (trap_index=0; trap_index<4; trap_index++) // check each of the four trap squares
                                    {
                                        if (BOARD(trap[trap_index])!=EMPTY_SQUARE)
                                        {
                                            capture=TRUE;
                                            for (k=0; k<4; k++)
                                            {
                                                if (OWNER(trap[trap_index])==OWNER(trap[trap_index]+direction[k]))
                                                {
                                                    capture=FALSE;
                                                }
                                            }
                                            if (capture)
                                            {
                                                ml[number_of_moves].capture_square[0]=trap[trap_index];
                                                ml[number_of_moves].captured_piece[0]=BOARD(trap[trap_index]);
                                                ml[number_of_moves].hashkey^=hashkey_piece[trap[trap_index]][BOARD(trap[trap_index])];
                                                BOARD(trap[trap_index])=EMPTY_SQUARE; // temporarily capture the piece, to avoid detecting same capture after step 2
                                            }
                                        }
                                    }
                                    BOARD(square)=BOARD(square+direction[i]); // temporarily move the second piece, to check for captures again
                                    BOARD(square+direction[i])=EMPTY_SQUARE;
                                    // check trap squares for capture
                                    for (trap_index=0; trap_index<4; trap_index++) // check each of the four trap squares
                                    {
                                        if (BOARD(trap[trap_index])!=EMPTY_SQUARE)
                                        {
                                            capture=TRUE;
                                            for (k=0; k<4; k++)
                                            {
                                                if (OWNER(trap[trap_index])==OWNER(trap[trap_index]+direction[k]))
                                                {
                                                    capture=FALSE;
                                                }
                                            }
                                            if (capture)
                                            {
                                                ml[number_of_moves].capture_square[1]=trap[trap_index];
                                                ml[number_of_moves].captured_piece[1]=BOARD(trap[trap_index]);
                                                ml[number_of_moves].hashkey^=hashkey_piece[trap[trap_index]][BOARD(trap[trap_index])];
                                            }
                                        }
                                    }
                                    BOARD(square+direction[i])=BOARD(square); // move the pieces back after we've checked for captures
                                    BOARD(square)=EMPTY_SQUARE;
                                    if (ml[number_of_moves].capture_square[0]) // if we temporarily captured a piece, put it back
                                    {
                                        BOARD(ml[number_of_moves].capture_square[0])=ml[number_of_moves].captured_piece[0];
                                    }
                                    BOARD(square)=BOARD(square+direction[j]);
                                    BOARD(square+direction[j])=EMPTY_SQUARE;
                                    number_of_moves++;
                                }
                            }
                            for (j=0; j<4; j++) // check if we can push it in one of the four directions
                            {
                                if (BOARD(square+direction[i]+direction[j])==EMPTY_SQUARE) // empty square found, so we can push the piece there
                                {
                                    ml[number_of_moves].steps=2;
                                    ml[number_of_moves].pass=FALSE;
                                    ml[number_of_moves].piece[0]=BOARD(square+direction[i]);
                                    ml[number_of_moves].from[0]=square+direction[i];
                                    ml[number_of_moves].to[0]=square+direction[i]+direction[j];
                                    ml[number_of_moves].piece[1]=BOARD(square);
                                    ml[number_of_moves].from[1]=square;
                                    ml[number_of_moves].to[1]=square+direction[i];
                                    ml[number_of_moves].hashkey=hashkey_piece[square+direction[i]][BOARD(square+direction[i])]^hashkey_piece[square+direction[i]+direction[j]][BOARD(square+direction[i])]
                                        ^hashkey_piece[square][BOARD(square)]^hashkey_piece[square+direction[i]][BOARD(square)];
                                    BOARD(square+direction[i]+direction[j])=BOARD(square+direction[i]); // temporarily move the first piece to check for captures
                                    BOARD(square+direction[i])=EMPTY_SQUARE;
                                    // check trap squares for capture
                                    ml[number_of_moves].capture_square[0]=0;
                                    ml[number_of_moves].capture_square[1]=0;
                                    for (trap_index=0; trap_index<4; trap_index++) // check each of the four trap squares
                                    {
                                        if (BOARD(trap[trap_index])!=EMPTY_SQUARE)
                                        {
                                            capture=TRUE;
                                            for (k=0; k<4; k++)
                                            {
                                                if (OWNER(trap[trap_index])==OWNER(trap[trap_index]+direction[k]))
                                                {
                                                    capture=FALSE;
                                                }
                                            }
                                            if (capture)
                                            {
                                                ml[number_of_moves].capture_square[0]=trap[trap_index];
                                                ml[number_of_moves].captured_piece[0]=BOARD(trap[trap_index]);
                                                ml[number_of_moves].hashkey^=hashkey_piece[trap[trap_index]][BOARD(trap[trap_index])];
                                                BOARD(trap[trap_index])=EMPTY_SQUARE; // temporarily capture the piece, to avoid detecting same capture after step 2
                                            }
                                        }
                                    }
                                    BOARD(square+direction[i])=BOARD(square); // temporarily move the second piece to check for captures again
                                    BOARD(square)=EMPTY_SQUARE;
                                    // check trap squares for capture
                                    for (trap_index=0; trap_index<4; trap_index++) // check each of the four trap squares
                                    {
                                        if (BOARD(trap[trap_index])!=EMPTY_SQUARE)
                                        {
                                            capture=TRUE;
                                            for (k=0; k<4; k++)
                                            {
                                                if (OWNER(trap[trap_index])==OWNER(trap[trap_index]+direction[k]))
                                                {
                                                    capture=FALSE;
                                                }
                                            }
                                            if (capture)
                                            {
                                                ml[number_of_moves].capture_square[1]=trap[trap_index];
                                                ml[number_of_moves].captured_piece[1]=BOARD(trap[trap_index]);
                                                ml[number_of_moves].hashkey^=hashkey_piece[trap[trap_index]][BOARD(trap[trap_index])];
                                            }
                                        }
                                    }
                                    BOARD(square)=BOARD(square+direction[i]); // move the pieces back after we've checked for captures
                                    BOARD(square+direction[i])=EMPTY_SQUARE;
                                    if (ml[number_of_moves].capture_square[0]) // if we temporarily captured a piece, put it back
                                    {
                                        BOARD(ml[number_of_moves].capture_square[0])=ml[number_of_moves].captured_piece[0];
                                    }
                                    BOARD(square+direction[i])=BOARD(square+direction[i]+direction[j]);
                                    BOARD(square+direction[i]+direction[j])=EMPTY_SQUARE;
                                    number_of_moves++;
                                }
                            }
                        }
                    }
                }
                // generate single moves
                for (i=0; i<4; i++) // check each direction
                {
                    if (BOARD(square+direction[i])==EMPTY_SQUARE) // if there's an empty square, we can move there
                    {
                        ml[number_of_moves].steps=1;
                        ml[number_of_moves].pass=FALSE;
                        ml[number_of_moves].piece[0]=BOARD(square);
                        ml[number_of_moves].from[0]=square;
                        ml[number_of_moves].to[0]=square+direction[i];
                        ml[number_of_moves].hashkey=hashkey_piece[square][BOARD(square)]^hashkey_piece[square+direction[i]][BOARD(square)];
                        BOARD(square+direction[i])=BOARD(square); // temporarily move the piece, to check for captures
                        BOARD(square)=EMPTY_SQUARE;
                        // check trap squares for capture
                        ml[number_of_moves].capture_square[0]=0;
                        ml[number_of_moves].capture_square[1]=0;
                        for (trap_index=0; trap_index<4; trap_index++) // check each of the four trap squares
                        {
                            if (BOARD(trap[trap_index])!=EMPTY_SQUARE)
                            {
                                capture=TRUE;
                                for (j=0; j<4; j++)
                                {
                                    if (OWNER(trap[trap_index])==OWNER(trap[trap_index]+direction[j]))
                                    {
                                        capture=FALSE;
                                    }
                                }
                                if (capture)
                                {
                                    ml[number_of_moves].capture_square[0]=trap[trap_index];
                                    ml[number_of_moves].captured_piece[0]=BOARD(trap[trap_index]);
                                    ml[number_of_moves].hashkey^=hashkey_piece[trap[trap_index]][BOARD(trap[trap_index])];
                                }
                            }
                        }
                        BOARD(square)=BOARD(square+direction[i]); // move the piece back after we've checked for captures
                        BOARD(square+direction[i])=EMPTY_SQUARE;
                        number_of_moves++;
                        if (PIECE(square)==RABBIT_PIECE) // If it's a rabbit, check so we're not moving it backwards
                        {
                            if (OWNER(square)==GOLD && direction[i]==SOUTH)
                            {
                                number_of_moves--;
                            }
                            if (OWNER(square)==SILVER && direction[i]==NORTH)
                            {
                                number_of_moves--;
                            }
                        }
                    }
                }
            }
        }
    }
    if (bp->steps>0) // we've moved at least one step, so passing is an option
    {
        ml[number_of_moves].steps=4-bp->steps;
        ml[number_of_moves].pass=TRUE;
        ml[number_of_moves].hashkey=0x0ULL;
        number_of_moves++;
    }
    return number_of_moves;
}

void BOARD_Print_Move(move_t *mp)
{
    int i;
    
    if (mp->pass)
    {
        sprintf(message,"(pass)");
        BOARD_Message();
    } else
    {
        for (i=0; i<mp->steps; i++)
        {
            if (i==1)
            {
                sprintf(message," ");
                BOARD_Message();
            }
            switch (mp->piece[i])
            {
                case (GOLD | ELEPHANT_PIECE) :
                    sprintf(message,"E");
                    BOARD_Message();
                    break;
                case (GOLD | CAMEL_PIECE) :
                    sprintf(message,"M");
                    BOARD_Message();
                    break;
                case (GOLD | HORSE_PIECE) :
                    sprintf(message,"H");
                    BOARD_Message();
                    break;
                case (GOLD | DOG_PIECE) :
                    sprintf(message,"D");
                    BOARD_Message();
                    break;
                case (GOLD | CAT_PIECE) :
                    sprintf(message,"C");
                    BOARD_Message();
                    break;
                case (GOLD | RABBIT_PIECE) :
                    sprintf(message,"R");
                    BOARD_Message();
                    break;
                case (SILVER | ELEPHANT_PIECE) :
                    sprintf(message,"e");
                    BOARD_Message();
                    break;
                case (SILVER | CAMEL_PIECE) :
                    sprintf(message,"m");
                    BOARD_Message();
                    break;
                case (SILVER | HORSE_PIECE) :
                    sprintf(message,"h");
                    BOARD_Message();
                    break;
                case (SILVER | DOG_PIECE) :
                    sprintf(message,"d");
                    BOARD_Message();
                    break;
                case (SILVER | CAT_PIECE) :
                    sprintf(message,"c");
                    BOARD_Message();
                    break;
                case (SILVER | RABBIT_PIECE) :
                    sprintf(message,"r");
                    BOARD_Message();
                    break;
                default :
                    sprintf(message,"Unknown or empty board square contents encountered while printing move.\n");
                    BOARD_Message();
                    break;
            }
            PRINT_SQUARE(mp->from[i]);
            switch (mp->to[i]-mp->from[i])
            {
                case NORTH :
                    sprintf(message,"n");
                    BOARD_Message();
                    break;
                case WEST :
                    sprintf(message,"w");
                    BOARD_Message();
                    break;
                case EAST :
                    sprintf(message,"e");
                    BOARD_Message();
                    break;
                case SOUTH :
                    sprintf(message,"s");
                    BOARD_Message();
                    break;
                default :
                    sprintf(message,"Unknown direction encountered while printing move.\n");
                    BOARD_Message();
                    break;
            }
            if (mp->capture_square[i])
            {
                sprintf(message," ");
                BOARD_Message();
                switch (mp->captured_piece[i])
                {
                    case (GOLD | ELEPHANT_PIECE) :
                        sprintf(message,"E");
                        BOARD_Message();
                        break;
                    case (GOLD | CAMEL_PIECE) :
                        sprintf(message,"M");
                        BOARD_Message();
                        break;
                    case (GOLD | HORSE_PIECE) :
                        sprintf(message,"H");
                        BOARD_Message();
                        break;
                    case (GOLD | DOG_PIECE) :
                        sprintf(message,"D");
                        BOARD_Message();
                        break;
                    case (GOLD | CAT_PIECE) :
                        sprintf(message,"C");
                        BOARD_Message();
                        break;
                    case (GOLD | RABBIT_PIECE) :
                        sprintf(message,"R");
                        BOARD_Message();
                        break;
                    case (SILVER | ELEPHANT_PIECE) :
                        sprintf(message,"e");
                        BOARD_Message();
                        break;
                    case (SILVER | CAMEL_PIECE) :
                        sprintf(message,"m");
                        BOARD_Message();
                        break;
                    case (SILVER | HORSE_PIECE) :
                        sprintf(message,"h");
                        BOARD_Message();
                        break;
                    case (SILVER | DOG_PIECE) :
                        sprintf(message,"d");
                        BOARD_Message();
                        break;
                    case (SILVER | CAT_PIECE) :
                        sprintf(message,"c");
                        BOARD_Message();
                        break;
                    case (SILVER | RABBIT_PIECE) :
                        sprintf(message,"r");
                        BOARD_Message();
                        break;
                    default :
                        sprintf(message,"Unknown or empty board square contents encountered while printing move.\n");
                        BOARD_Message();
                        break;
                }
                PRINT_SQUARE(mp->capture_square[i]);
                sprintf(message,"x");
                BOARD_Message();
            }
        }
    }
}

void BOARD_Send_Move(move_t *mp)
{
    int i;
    
    if (mp->pass)
    {
        // in the case of a pass, send nothing!
    } else
    {
        for (i=0; i<mp->steps; i++)
        {
            if (i==1)
            {
                printf(" ");
            }
            switch (mp->piece[i])
            {
                case (GOLD | ELEPHANT_PIECE) :
                    printf("E");
                    break;
                case (GOLD | CAMEL_PIECE) :
                    printf("M");
                    break;
                case (GOLD | HORSE_PIECE) :
                    printf("H");
                    break;
                case (GOLD | DOG_PIECE) :
                    printf("D");
                    break;
                case (GOLD | CAT_PIECE) :
                    printf("C");
                    break;
                case (GOLD | RABBIT_PIECE) :
                    printf("R");
                    break;
                case (SILVER | ELEPHANT_PIECE) :
                    printf("e");
                    break;
                case (SILVER | CAMEL_PIECE) :
                    printf("m");
                    break;
                case (SILVER | HORSE_PIECE) :
                    printf("h");
                    break;
                case (SILVER | DOG_PIECE) :
                    printf("d");
                    break;
                case (SILVER | CAT_PIECE) :
                    printf("c");
                    break;
                case (SILVER | RABBIT_PIECE) :
                    printf("r");
                    break;
                default :
                    printf("Unknown or empty board square contents encountered while printing move.\n");
                    break;
            }
            printf("%c%c",COL(mp->from[i])-1+'a',ROW(mp->from[i])-1+'1');
            switch (mp->to[i]-mp->from[i])
            {
                case NORTH :
                    printf("n");
                    break;
                case WEST :
                    printf("w");
                    break;
                case EAST :
                    printf("e");
                    break;
                case SOUTH :
                    printf("s");
                    break;
                default :
                    printf("Unknown direction encountered while printing move.\n");
                    break;
            }
            if (mp->capture_square[i])
            {
                printf(" ");
                switch (mp->captured_piece[i])
                {
                    case (GOLD | ELEPHANT_PIECE) :
                        printf("E");
                        break;
                    case (GOLD | CAMEL_PIECE) :
                        printf("M");
                        break;
                    case (GOLD | HORSE_PIECE) :
                        printf("H");
                        break;
                    case (GOLD | DOG_PIECE) :
                        printf("D");
                        break;
                    case (GOLD | CAT_PIECE) :
                        printf("C");
                        break;
                    case (GOLD | RABBIT_PIECE) :
                        printf("R");
                        break;
                    case (SILVER | ELEPHANT_PIECE) :
                        printf("e");
                        break;
                    case (SILVER | CAMEL_PIECE) :
                        printf("m");
                        break;
                    case (SILVER | HORSE_PIECE) :
                        printf("h");
                        break;
                    case (SILVER | DOG_PIECE) :
                        printf("d");
                        break;
                    case (SILVER | CAT_PIECE) :
                        printf("c");
                        break;
                    case (SILVER | RABBIT_PIECE) :
                        printf("r");
                        break;
                    default :
                        printf("Unknown or empty board square contents encountered while printing move.\n");
                        break;
                }
                switch (mp->capture_square[i])
                {
                    case 33 :
                        printf("c6");
                        break;
                    case 36 :
                        printf("f6");
                        break;
                    case 63 :
                        printf("c3");
                        break;
                    case 66 :
                        printf("f3");
                        break;
                    default :
                        printf("Unknown capturing square encountered while printing move.\n");
                        break;
                }
                printf("x");
            }
        }
    }
}

void BOARD_Message_Init(void)
{
    #ifndef TESTING
    {
        fp=fopen("log.txt","a");
    }
    #endif
}

void BOARD_Message(void)
{
    #ifndef TESTING
    {
        fprintf(stderr,"%s",message);
        fprintf(fp,"%s",message);
    }
    #endif
}

void BOARD_Message_Exit(void)
{
    #ifndef TESTING
    {
        fclose(fp);
    }
    #endif
}

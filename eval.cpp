#include "eval.h"

//---------------------------------------------------------------------
//  section Eval
//---------------------------------------------------------------------- 

int Eval::evaluate(const Board* board_)
{
  static const int piece_value[7]={0,RABBIT_VALUE,CAT_VALUE,DOG_VALUE,HORSE_VALUE,CAMEL_VALUE,ELEPHANT_VALUE};
  int eval[2] = {0,0};

  for ( uint i = 0; i < 2; i++){
    for ( uint j = 0; j < board_->pieceArray[i].getLen(); j++){
      assert(PIECE(board_->board_[board_->pieceArray[i][j]]) > 0 && PIECE(board_->board_[board_->pieceArray[i][j]]) < 7);
      eval[i] += piece_value[PIECE(board_->board_[board_->pieceArray[i][j]])];
  //  cerr << i << "+ " << piece_value[PIECE(board_[pieceArray[i][j]])] << endl;
    }
   // cerr <<endl;
  }
  return eval[0] - eval[1];
}

//--------------------------------------------------------------------- 

float Eval::evaluateInPercent(const Board* board_) 
{
  int evaluation = evaluateBetter(board_);

  //TODO mapping evaluation -> win percentage is VERY DUMMY - improve
  if (evaluation > 0)
    return 1;
  else 
    return 0;
}

//--------------------------------------------------------------------- 

#define RABBIT_FREE_AHEAD 1000
#define RABBIT_FRIENDLY_AHEAD 500
#define RABBIT_FREE_SIDE 300
#define RABBIT_FRIENDLY_SIDE 200

// board constants
static const int trap_square[4]={33,36,63,66};

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
                                                                    
// Evaluation is done from gold's perspective.  At the end of the evaluation, it's adjusted to be seen from current player's perspective.


int Eval::evaluateBetter(const Board* board)
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
      
  // trap evaluation variables
  int trap_adjacent[2];
  int trap_adjacent_strength[2];
  int trap_adjacent_strongest[2];

  //list<int> pieces[2][8];

  int rabbits[2]={0,0};
  int rabbit_pos[2][8];
                   
  // material evaluation variables
  int material[100]; // What is the piece on this square worth?
  int piece_frozen;
  int piece_adjacent_stronger_enemy;
  int piece_adjacent_empty;
  int piece_adjacent_strongest[2];
  int piece_adjacent[2];
  int piece_adjacent_trap;


  //position
  const board_t & p = board->board_;

  #define FOWNER(square) (int) OWNER(p[square])
  #define FPIECE(square) (int) PIECE(p[square])
  #define FBOARD(square) (int) p[square]
  
  // rabbit evaluation variables
  int row;
  
  // Initialize some evaluation stuff

  side_mask[GOLD]=0;
  side_mask[SILVER]=1;

  for ( uint i = 0; i < SQUARE_NUM; i++)
    material[i] = 0;
  
  for ( uint i = 0; i < 2; i++){
    for ( uint j = 0; j < board->pieceArray[i].getLen(); j++){
      int square = board->pieceArray[i][j];
      assert(FPIECE(square) > 0 && FPIECE(square) < 7);
      //pieces[i][FPIECE(square)].push_back(square);
      if (FPIECE(square) == PIECE_RABBIT){
        rabbit_pos[i][rabbits[i]++] = square;
      }
      material[square] = piece_value[FPIECE(square)];
    }
  }

  // Evaluate trap squares, decide trap ownership.
  for (trap=0; trap<4; trap++){
    for (side=0; side<2; side++){
      trap_adjacent[side]=0;
      trap_adjacent_strength[side]=0;
      trap_adjacent_strongest[side]=0;
    }
    for (dir=0; dir<4; dir++){
      switch (FOWNER(trap_square[trap]+direction[dir])){
        case GOLD :
          trap_adjacent[0]++;
          trap_adjacent_strength[0]+=FPIECE(trap_square[trap]+direction[dir]);
          if (FPIECE(trap_square[trap]+direction[dir])>trap_adjacent_strongest[0]){
            trap_adjacent_strongest[0]=FPIECE(trap_square[trap]+direction[dir]);
          }
          break;
        case SILVER :
          trap_adjacent[1]++;
          trap_adjacent_strength[1]+=FPIECE(trap_square[trap]+direction[dir]);
          if (FPIECE(trap_square[trap]+direction[dir])>trap_adjacent_strongest[1]){
            trap_adjacent_strongest[1]=FPIECE(trap_square[trap]+direction[dir]);
          }
          break;
      }
    }
    // Basically, 200 points are given out per trap.  50 to whoever has the strongest piece by the trap, 
    // and 150 points split according to total strength of pieces, with two neutral strength added.
    
    // case 1 - only one side has pieces by the trap.
    if (trap_adjacent[0]>0 && trap_adjacent[1]==0){
      trap_value[0]+=50+trap_adjacent_strength[0]*150/(trap_adjacent_strength[0]+1);
    }
    if (trap_adjacent[1]>0 && trap_adjacent[0]==0){
      trap_value[1]+=50+trap_adjacent_strength[1]*150/(trap_adjacent_strength[1]+1);
    }
    // case 2 - both sides have pieces by the trap.
    if (trap_adjacent[0]>0 && trap_adjacent[1]>0){
      // subcase 1 - they are equally strong.  Split 100 points according to number of pieces.
      if (trap_adjacent_strongest[0]==trap_adjacent_strongest[1]){
        trap_value[0]+=trap_adjacent_strength[0]*200/(trap_adjacent_strength[0]+trap_adjacent_strength[1]+1);
        trap_value[1]+=trap_adjacent_strength[1]*200/(trap_adjacent_strength[0]+trap_adjacent_strength[1]+1);
      }
      // subcase 2 - gold is stronger.  Give 50 points to gold, and split 50 according to number of pieces.
      if (trap_adjacent_strongest[0]>trap_adjacent_strongest[1]){
        trap_value[0]+=50+trap_adjacent_strength[0]*150/(trap_adjacent_strength[0]+trap_adjacent_strength[1]+1);
        trap_value[1]+=trap_adjacent_strength[1]*150/(trap_adjacent_strength[0]+trap_adjacent_strength[1]+1);
      }
      // subcase 3 - silver is stronger.  Give 50 points to silver, and split 50 according to number of pieces.
      if (trap_adjacent_strongest[1]>trap_adjacent_strongest[0]){
        trap_value[0]+=trap_adjacent_strength[0]*150/(trap_adjacent_strength[0]+trap_adjacent_strength[1]+1);
        trap_value[1]+=50+trap_adjacent_strength[1]*150/(trap_adjacent_strength[0]+trap_adjacent_strength[1]+1);
      }
    }

    // special case - give minus for (possible) frames
    if (FOWNER(trap_square[trap])==GOLD && trap_adjacent[1]>2){
      material[trap_square[trap]]=material[trap_square[trap]]*4/5; // Trapped piece loses 20% of its value
    }
    if (FOWNER(trap_square[trap])==SILVER && trap_adjacent[0]>2){
      material[trap_square[trap]]=material[trap_square[trap]]*4/5; // Trapped piece loses 20% of its value
    }
  }

  /* 
  // Evaluate material and individual pieces.

  for (side=0; side<2; side++){
    for (i=0; i<cats[side]; i++){
      if (side == 0)
        row=ROW(cat_pos[0][i]);
      else
        row=9-ROW(cat_pos[1][i]);
      if (row>3){
        material[cat_pos[side][i]]=material[cat_pos[side][i]]*197/200; // Advanced cat lose 1.5 % of its value
      } else if (row==3)
      {
        material[cat_pos[side][i]]=material[cat_pos[side][i]]*199/200; // Slightly advanced cat lose 0.5 % of its value
      }
    }
  
    for (i=0; i<dogs[side]; i++){
      if (side == 0){
          row=ROW(dog_pos[0][i]);
      }
      else{
          row=9-ROW(dog_pos[1][i]);
      }
      if (row>3){
        material[dog_pos[side][i]]=material[dog_pos[side][i]]*197/200; // Advanced dog lose 1.5 % of its value
      } else if (row==3)
      {
        material[dog_pos[side][i]]=material[dog_pos[side][i]]*199/200; // Slightly advanced dog lose 0.5 % of its value
      }
    }
  }
  */

  for (square=11; square<=88; square++)
  {  
    if (square%10==9) square+=2;
    if (FOWNER(square)==GOLD || FOWNER(square)==SILVER)
    {
      // Check if it's frozen, number of adjacent empty, strongest adjacent, and all that
      piece_adjacent[0]=0;
      piece_adjacent[1]=0;
      piece_adjacent_empty=0;
      piece_adjacent_strongest[0]=0;
      piece_adjacent_strongest[1]=0;
      for (dir=0; dir<4; dir++)
      {
        switch (FOWNER(square+direction[dir]))
        {
          case GOLD :
            piece_adjacent[0]++;
            if (FPIECE(square+direction[dir])>piece_adjacent_strongest[0])
            {
              piece_adjacent_strongest[0]=FPIECE(square+direction[dir]);
            }
            break;
          case SILVER :
            piece_adjacent[1]++;
            if (FPIECE(square+direction[dir])>piece_adjacent_strongest[1])
            {
              piece_adjacent_strongest[1]=FPIECE(square+direction[dir]);
            }
            break;
          case EMPTY :
            piece_adjacent_empty++;
            break;
        }
      }
      if (FOWNER(square) == GOLD){
          piece_adjacent_stronger_enemy=piece_adjacent_strongest[1]>FPIECE(square);
          piece_frozen=piece_adjacent_stronger_enemy && piece_adjacent[0]==0;
      }
      else { 
          piece_adjacent_stronger_enemy=piece_adjacent_strongest[0]>FPIECE(square);
          piece_frozen=piece_adjacent_stronger_enemy && piece_adjacent[1]==0;
      }
      if (piece_frozen)
      {
        material[square]=material[square]*9/10; // Frozen piece loses 10% of its value
      }
      if (piece_adjacent_empty==0) 
      {
        material[square]=material[square]*199/200; // Immobile piece loses 0.5% of its value
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
            if (FOWNER(adjacent_trap[square]+direction[dir])==FOWNER(square))
            {
              piece_adjacent_trap++;
            }
          }
          if (piece_adjacent_trap==1)
          {
            material[square]=material[square]*7/10;
          }
        }
        if (adjacent2_trap[square] && FBOARD(adjacent2_trap[square])==EMPTY_SQUARE) 
        // It's two steps away from an empty trap
        {
          // If we have no piece next to the trap,
          // Really - should check so that there is a free path to trap.
          // then consider this piece to be threatened, losing 30% of its value
          piece_adjacent_trap=0;
          for (dir=0; dir<4; dir++)
          {
            if (FOWNER(adjacent2_trap[square]+direction[dir])==FOWNER(square))
            {
              piece_adjacent_trap++;
            }
          }
          if (piece_adjacent_trap==0)
          {
            material[square]=material[square]*7/10;
          }
        }
      }
      // Another case - if adjacent to a trap, and no other friendly piece adjacent, various possibilities for being threatened....
      switch (FOWNER(square))
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
      switch (FOWNER(rabbit_pos[0][i]+NORTH))
      {
        case EMPTY :
          rabbit_value[0]+=RABBIT_FREE_AHEAD;
          break;
        case GOLD :
          rabbit_value[0]+=RABBIT_FRIENDLY_AHEAD;
          break;
      }
      switch (FOWNER(rabbit_pos[0][i]+EAST))
      {
        case EMPTY :
          rabbit_value[0]+=RABBIT_FREE_SIDE;
          break;
        case GOLD :
          rabbit_value[0]+=RABBIT_FRIENDLY_SIDE;
          break;
      }
      switch (FOWNER(rabbit_pos[0][i]+WEST))
      {
        case EMPTY :
          rabbit_value[0]+=RABBIT_FREE_SIDE;
          break;
        case GOLD :
          rabbit_value[0]+=RABBIT_FRIENDLY_SIDE;
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
      switch (FOWNER(rabbit_pos[1][i]+SOUTH))
      {
        case EMPTY :
          rabbit_value[1]+=RABBIT_FREE_AHEAD;
          break;
        case SILVER :
          rabbit_value[1]+=RABBIT_FRIENDLY_AHEAD;
          break;
      }
      switch (FOWNER(rabbit_pos[1][i]+EAST))
      {
        case EMPTY :
          rabbit_value[1]+=RABBIT_FREE_SIDE;
          break;
        case SILVER :
          rabbit_value[1]+=RABBIT_FRIENDLY_SIDE;
          break;
      }
      switch (FOWNER(rabbit_pos[1][i]+WEST))
      {
        case EMPTY :
          rabbit_value[1]+=RABBIT_FREE_SIDE;
          break;
        case SILVER :
          rabbit_value[1]+=RABBIT_FRIENDLY_SIDE;
          break;
      }
    }
  }

  //add all up
  value+=material_value[0]-material_value[1];
  value+=trap_value[0]-trap_value[1];
  value+=rabbit_value[0]-rabbit_value[1];

  //REMOVE
  return value;
}
  
/**/

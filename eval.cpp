#include "eval.h"

int Eval::evaluate(const Board* board_){
  static const int piece_value[7]={0,RABBIT_VALUE,CAT_VALUE,DOG_VALUE,HORSE_VALUE,CAMEL_VALUE,ELEPHANT_VALUE};
  int eval[2] = {0,0};

  for ( uint i = 0; i < 2; i++){
    for ( uint j = 0; j < board_->pieceArray[i].getLen(); j++){
      assert(PIECE(board_->board_[board_->pieceArray[i][j]]) > 0 && PIECE(board_->board_[board_->pieceArray[i][j]]) < 7);
      eval[i] += piece_value[PIECE(board_->board_[board_->pieceArray[i][j]])];
  //    cerr << i << "+ " << piece_value[PIECE(board_[pieceArray[i][j]])] << endl;
    }
   // cerr <<endl;
  }
  return eval[0] - eval[1];
}


float Eval::evaluateInPercent(const Board* board_) 
{
  int evaluation = evaluate(board_);

  //TODO mapping evaluation -> win percentage is VERY DUMMY - improve
  if (evaluation > 0)
    return 0.9;
  else 
    return 0.1;
}

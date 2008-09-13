
#include "board.h"

using namespace BitStuff;

bit64 getNeighbours(bit64 pieces)
  //Returns neighbors bitset for a bitset of pieces
{
  bit64   result;

  result =  (pieces & notHfile) << 1;
  result |= (pieces & notAfile) >> 1;
  result |= (pieces & not1rank) << 8;
  result |= (pieces & not8rank) >> 8;

  return(result);
}

void Move::setValues( moveType_t moveType, color_t color, piece_t piece, coord_t from, coord_t to)
{
moveType_ = moveType;
color_		= color;
piece_		= piece;
from_			= from;
to_				= to;
}

void Move::setValues( moveType_t moveType, color_t color, piece_t piece, coord_t from, coord_t to, 
						piece_t oppPiece, coord_t oppFrom, coord_t oppTo)
{
moveType_ = moveType;
color_ = color;
piece_ = piece;
from_ = from;
to_ = to;
oppPiece_ = oppPiece;
oppFrom_ = oppFrom;
oppTo_ = oppTo;

}
const string Move::getStepStr(color_t color, piece_t piece, coord_t from, coord_t to) 
	//prints step string for given values
{
	stringstream s;
	string pieceRefStr(" RCDHMErcdhme");
	string columnRefStr("abcdefgh");

	s << pieceRefStr[piece + 6 * color] << columnRefStr[from % 8] << from / 8 + 1;	
  switch (to - from)
  {
    case NORTH : s << "n"; break;
    case WEST :  s << "w"; break;
    case EAST :  s << "e"; break;
    case SOUTH : s << "s"; break;
    default :
			assert(false);
	}
	s << " ";
	return s.str();
	
}

void Move::dump()
{
	
	log_() << "(";
	switch (moveType_) {
		case MOVE_PASS: 
			log_() << "pass";			
			break;
		case MOVE_SINGLE: 
			log_() << getStepStr(color_, piece_, from_, to_) ; 
			break;
		case MOVE_PUSH: 
			log_() << getStepStr(1-color_, oppPiece_, oppFrom_, oppTo_)
						 << getStepStr(color_, piece_, from_, to_ );
			break;
		case MOVE_PULL: 
			log_() << getStepStr(color_, piece_, from_, to_ )
						 << getStepStr(1-color_, oppPiece_, oppFrom_, oppTo_);
			break;
		default:
			assert(false);

	}
	log_() << ") ";
}

void Board::test()
{
	for (int color = 0; color < 2; color++)
		for (int piece = 0; piece < 7; piece++){
			log_() << "piece: " << piece << endl;
			for ( int j = 0; j < BIT_LEN; j ++ ) {
				log_() << "position: " << j << endl;
				for ( int i = 0; i < BIT_LEN; i ++ ){
					log_() << moveOffset_[color][piece][j][i] ;
					if ( i% 8 == 7 )
						log_() << endl;
				}
			}
		}
}

void Board::build_move_offsets()
   //This table returns a bitmap of legal squares for a
   //given piece on a given color on a given square.
{
  int  color;
  int  piece;
  int  square;

  // do rabbits as a special case
  // --
  for (color = 0; color < 2; color++)
    for (square = 0; square < BIT_LEN; square++) {
      bit64  ts;
      assert( ts.none() );

      
      ts |= ((one << square) & notHfile) << 1;  // east
      ts |= ((one << square) & notAfile) >> 1;  // west
      if (color == 0)
        ts |= ((one << square) & not8rank) >> 8;  // north
      else
        ts |= ((one << square) & not1rank) << 8;  // south

      moveOffset_[color][RABBIT][square] = ts;    
    }

  // Now do the rest
  for (color = 0; color < 2; color++)
    for (piece = 2; piece < 7; piece++)
      for (square = 0; square < BIT_LEN; square++) {
          bit64  ts;
          assert( ts.none());

          ts |= ((one << square) & notHfile) << 1;  // east
          ts |= ((one << square) & notAfile) >> 1;  // west
          ts |= ((one << square) & not1rank) << 8;  // south
          ts |= ((one << square) & not8rank) >> 8;  // north

          moveOffset_[color][piece][square] = ts;
      }

}

bool Board::isEmpty() 
{
 return moveCnt_ == 1;
}

bool Board::isGoldMove() 
{
 return toMove_ == GOLD;
}


int Board::generateMoves(MoveList& moveList)
	/*Crucial function generating moves from position for player to Move.
	 *
	 * returns number of generated moves saved in moveList array from index 0,
	 * fixed array is used as a datastructure for performance reasons ( instead of 
	 * for instance dynamic list )*/
{
  int tm = toMove_;																				//player to move
	int listCount = 0;
  bit64		empty(~(bitBoard_[0][0] | bitBoard_[1][0]));		// mask of unoccupied squares    
  bit64   notFrozenSquares;																// mask of friendly pieces which are not frozen
  bit64   movablePieces;																	// mask of friendly pieces on notFrozenSquares
  bit64		stronger (bitBoard_[1-tm][0]);                  // mask of stronger enemy pcs for movablePieces
  bit64		weaker;																			 // weaker enemy pieces all over the board 
	bit64		victims;																		 // weaker enemy pieces in adjacent squares
	bit64		wherePush;																	 // where to push
	bit64		wherePull;																	 // where to pull
  bit64   whereMove;																	 // where to move    

	//movements
	//onestep: (piece) fromSquare-> toSquare 
	//push: (piece) fromSquare->victimFromSquare, (victim) victimFromSquare->victimToSquare
	//pull: (piece) fromSquare->pullertoSquare, (victim) victimFromSquare->fromSquare
	int fromSquare;
	int toSquare;
	int victimFromSquare;
	int victimToSquare;														 
	int pullerToSquare;

  for (int piece = 1; piece < 7; piece++) {
    movablePieces = bitBoard_[tm][piece];														// get all pieces of this type
    stronger ^= bitBoard_[1-tm][piece];															// remove from consideration
    notFrozenSquares = 
			getNeighbours(bitBoard_[tm][0]) | (~getNeighbours(stronger)); //around is friendly piece or no stroger enemy piece 
    movablePieces &= notFrozenSquares;															// pieces from slice on notFrozenSquares

		weaker |= bitBoard_[1-tm][piece-1];														// all pieces we can we can push/pull
		
    fromSquare = movablePieces._Find_first();							// consider moves from this square next 
    while (fromSquare != BIT_LEN) {

      whereMove = empty & moveOffset_[tm][piece][fromSquare];
      victims = weaker & moveOffset_[tm][piece][fromSquare];

			//generate one step moves
      toSquare = whereMove._Find_first();						 // get next whereMove move  
      while (toSquare != BIT_LEN) {

				moveList[listCount++].setValues(MOVE_SINGLE, tm, piece, fromSquare, toSquare);

				toSquare = whereMove._Find_next(toSquare);						 // get next whereMove move  
      }

			if ( piece > RABBIT ) {
				//generate push/pull moves
				victimFromSquare = victims._Find_first();												// get next victim 
				while (victimFromSquare != BIT_LEN) {
					assert(piece != RABBIT );																			// rabbits can't have victims :)
					wherePush = empty & moveOffset_[0][piece][victimFromSquare]; 
					wherePull = empty & moveOffset_[0][piece][fromSquare];					// optimize: out of the while cycle

					pullerToSquare = wherePull._Find_first();											// where our piece (puller) can retrieve
					while (pullerToSquare != BIT_LEN) {

						moveList[listCount++].setValues(MOVE_PULL, tm, piece, fromSquare, pullerToSquare, 
																			getSquarePiece(victimFromSquare), victimFromSquare, fromSquare );	
						pullerToSquare = wherePull._Find_next(pullerToSquare);			 // where our piece (puller) can retrieve
					}

					victimToSquare = wherePush._Find_first();												// where victim can be pushed to
					while (victimToSquare!=BIT_LEN) {

						moveList[listCount++].setValues(MOVE_PUSH, tm, piece, fromSquare, victimFromSquare,
																			getSquarePiece(victimFromSquare), victimFromSquare, victimToSquare  );
						victimToSquare = wherePush._Find_next(victimToSquare);												// where victim can be pushed to
					}
				victimFromSquare = victims._Find_next(victimFromSquare);					// get next victim 
				} //while victims
			}//if piece rabbit
    fromSquare = movablePieces._Find_next(fromSquare);							// consider moves from this square next 
		} //while movablePieces any
	} // for piece

	return listCount;
} //Board::generateMoves

bool Board::init(const char* fn)
 /**
 * Inits position from a method in file.
 *
 * returns true if initialization went right 
 * otherwise returns false
 */
{

  /* clear board */
  for (int i=0; i<7; i++) {
    bitBoard_[0][i].reset();
		bitBoard_[1][i].reset();
  }

  fstream f;
  char side;
  char c;

  try { 
     f.open(fn,fstream::in);
    if (! f.good()){
      f.close();
       return false;
     }

    f >> moveCnt_; 
    f >> side;
    f.ignore(1024,'\n'); //ignores the rest of initial line 

    if (side == 'w')
      toMove_ = GOLD;
    else {
      toMove_ = SILVER;
      moveCnt_++; //TODO ? 
    }

    f.ignore(1024,'\n'); //ignores the top of the border till EOF 

    for (int i = 0; i < 8; i++) { // do this for each of the 8 lines of the board
      f.ignore(2); //ignore trailing characters

      for (int j = 0; j < 8; j++) {
        f.ignore(1); //ignore a white space 
         c=f.get();

         switch(c) {
           case 'E' : setSquare(i*8+j, GOLD, ELEPHANT);  break;
           case 'M' : setSquare(i*8+j, GOLD, CAMEL);   break;
           case 'H' : setSquare(i*8+j, GOLD, HORSE);   break;
           case 'D' : setSquare(i*8+j, GOLD, DOG);    break;
           case 'C' : setSquare(i*8+j, GOLD, CAT);    break;
           case 'R' : setSquare(i*8+j, GOLD, RABBIT);   break;
           case 'e' : setSquare(i*8+j, SILVER, ELEPHANT); break;
           case 'm' : setSquare(i*8+j, SILVER, CAMEL);  break;
           case 'h' : setSquare(i*8+j, SILVER, HORSE);  break;
           case 'd' : setSquare(i*8+j, SILVER, DOG);   break;
           case 'c' : setSquare(i*8+j, SILVER, CAT);   break;
           case 'r' : setSquare(i*8+j, SILVER, RABBIT);  break;
					 case ' ' : case 'X' : case 'x': break; //setSquare(i*10+j, NO_COLOR, EMPTY); break;
           default :
           log_() << "Unknown character " << c << " encountered while reading board at [" << i << "," << j << "].\n";
           f.close();
           return false;
           break;
         }
      } //for
      f.ignore(1024,'\n'); //finish the line
    } //for 
      f.close();
  } //try
  catch(int e) {
    return false; //initialization from file failed
  }

  build_move_offsets();

  return true;
}


void Board::dump() 
{

  log_() << endl;
  log_() << "Move " << moveCnt_ / 2 + 1 << endl;// << ", Step " << stepCnt_ << ", ";

  if (toMove_ == GOLD) 
    log_() << "Gold to move." << endl;
  else
    log_() << "Silver to move." << endl;
  assert(toMove_ == GOLD || toMove_ == SILVER );

  log_() << " +-----------------+\n";

		
	string refStr(".123456rcdhmeRCDHME");
	for (int i = 0; i < BIT_LEN; i++) {
			if ( i % 8 == 0 ) 
				log_() << i / 8 + 1 <<"| ";
			assert(getSquarePiece(i) + ((2-getSquareColor(i)) * 6) <= refStr.length() );
			log_() << refStr[getSquarePiece(i) + ((2-getSquareColor(i)) * 6)] << " ";
			if ( i % 8 == 7 ) 
				log_() << "| " << endl;
	}


  log_() << " +-----------------+" << endl;
  log_() << "   a b c d e f g h" << endl;

	//for (int color = 0; color < 2; color++)
	//	for (int piece = 0; piece < 7; piece++) 
	//		log_() << bitBoard_[color][piece] << endl;
  //log_() << "Hashkey: %#.8X%.8X\n",(unsigned long int)(bp->hashkey>>32),(unsigned long int)(bp->hashkey&0xFFFFFFFFULL);
	
  } //Board::dump

void Board::setSquare(coord_t coord, color_t color, piece_t piece) 
{
 //bitBoard is [2][7] array of bit64
 assert(color == 1 || color == 0);
 assert(piece >= 1 && piece < 7 );
 assert(coord >= 0 && coord < BIT_LEN );
 
 bitBoard_[color][piece].set(coord);
 bitBoard_[color][0].set(coord);
}

piece_t Board::getSquarePiece(coord_t coord) 
{
  if (bitBoard_[GOLD][0][coord])
		for (int i = 1; i < 7; i++ )
			if (bitBoard_[GOLD][i][coord])
				return i;

  if (bitBoard_[SILVER][0][coord])
    for (int i = 1; i < 7; i++ )
			if (bitBoard_[SILVER][i][coord])
				return i;

  return EMPTY;
}

color_t Board::getSquareColor(coord_t coord) 
{
  if (bitBoard_[GOLD][0][coord])
		return GOLD;

  if (bitBoard_[SILVER][0][coord])
    return SILVER;

  return NO_COLOR;
}


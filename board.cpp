
#include "board.h"

using namespace BitStuff;

unsigned int  find_first(bit64 b)
{
  int  i=0;

  while (b.any())
  {
    if (b[0]) return(i);
    b >>= 1;
		i++;
  }

  return(-1);
}

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

Move::Move( moveType_t moveType, color_t color, piece_t piece, coord_t from, coord_t to):
					moveType_(moveType), color_(color), piece_(piece), from_(from), to_(to)
{
}

Move::Move( moveType_t moveType, color_t color, piece_t piece, coord_t from, coord_t to, 
						piece_t oppPiece, coord_t oppFrom, coord_t oppTo): 
					moveType_(moveType), color_(color), piece_(piece), from_(from), to_(to),
					oppPiece_(oppPiece), oppFrom_(oppFrom), oppTo_(oppTo)
{
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
		case MOVE_PULL: 
			log_() << getStepStr(color_, piece_, from_, to_ )
						 << getStepStr(1-color_, oppPiece_, oppFrom_, oppTo_);
			break;
		default:
			assert(false);

	}
}

void Board::test()
{
	for (int color = 0; color < 2; color++)
		for (int piece = 0; piece < 7; piece++){
			log_() << "piece: " << piece << endl;
			for ( int j = 0; j < 64; j ++ ) {
				log_() << "position: " << j << endl;
				for ( int i = 0; i < 64; i ++ ){
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
    for (square = 0; square < 64; square++) {
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
      for (square = 0; square < 64; square++) {
          bit64  ts;
          assert( ts.none());

          ts |= ((one << square) & notHfile) << 1;  // east
          ts |= ((one << square) & notAfile) >> 1;  // west
          ts |= ((one << square) & not1rank) << 8;  // south
          ts |= ((one << square) & not8rank) >> 8;  // north
    /*    ts |= (BIT_ON(square) & NOT_H_FILE) << 1;  // east
          ts |= (BIT_ON(square) & NOT_A_FILE) >> 1;  // west
          ts |= (BIT_ON(square) & NOT_1_RANK) << 8;  // south
          ts |= (BIT_ON(square) & NOT_8_RANK) >> 8;  // north
					*/

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

void Board::generatePushMoves(MoveList& moveList)
{
}

void Board::generatePullMoves(MoveList& moveList)
{
}


void Board::generateOneStepMoves(MoveList& moveList)
{
  int tm = toMove_;                                    //player to move
  bit64 empty(~(bitBoard_[0][0] | bitBoard_[1][0]));   // mask of unoccupied squares    
  bit64   notFrozen;                                   // mask of pieces which are not frozen
  bit64   potPieces;																	 // mask of potential pieces to move
  bit64 stronger (bitBoard_[1-tm][0]);                 // mask of stronger enemy pcs for potPieces

  for (int piece = 1; piece < 7; piece++) {
    potPieces = bitBoard_[tm][piece];									// get all pieces of this type
    stronger ^= bitBoard_[1-tm][piece];								// remove from consideration
    notFrozen = getNeighbours(bitBoard_[tm][0]) | (~getNeighbours(stronger));
    potPieces &= notFrozen;                           // bitmap of unfrozen pieces - potPiecess to move

    while (potPieces.any()) {
      bit64    potSquares;                            // potential squares to move to for a single item in potpieces     
      int    fromSquare = find_first(potPieces);      // consider moves from this square next 

      potPieces.reset(fromSquare);                    // cancel out for next pass             
      potSquares = empty & moveOffset_[tm][piece][fromSquare];

      while (potSquares.any()) {
        int  toSquare = find_first(potSquares);       // get next potPieces move  
        potSquares.reset(toSquare);                   // cancel out              

				Move * move = new Move(MOVE_SINGLE, tm, piece, fromSquare, toSquare);
				moveList.push_back(move);
      }
    }
  }
}

void Board::generateMoves(MoveList& moveList) 
{
	assert(moveList.empty());

  generatePullMoves(moveList);
  generatePushMoves(moveList);
  generateOneStepMoves(moveList);
}



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
           case ' ' : case 'X' : break; //setSquare(i*10+j, NO_COLOR, EMPTY); break;
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
 assert(coord >= 0 && coord < 64 );
 
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



#include "board.h"

bool Board::isEmpty() 
{
 return moveCnt_ == 1;
}

bool Board::isGoldMove() 
{
 return toMove_ == GOLD;
}


bool Board::init(const char* fn)
 /**
 * Inits position from a method in file.
 *
 * returns true if initialization went right 
 * otherwise returns false
 */
{
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

  for (int i = 1; i < 9; i++) { // do this for each of the 8 lines of the board
   f.ignore(2); //ignore trailing characters

   for (int j = 1; j < 9; j++) {
   f.ignore(1); //ignore a white space 
    c=f.get();

    switch(c) {
     case 'E' : setSquare(i*10+j,GOLD | ELEPHANT_PIECE);  break;
     case 'M' : setSquare(i*10+j,GOLD | CAMEL_PIECE);   break;
     case 'H' : setSquare(i*10+j,GOLD | HORSE_PIECE);   break;
     case 'D' : setSquare(i*10+j,GOLD | DOG_PIECE);    break;
     case 'C' : setSquare(i*10+j,GOLD | CAT_PIECE);    break;
     case 'R' : setSquare(i*10+j,GOLD | RABBIT_PIECE);   break;
     case 'e' : setSquare(i*10+j,SILVER | ELEPHANT_PIECE); break;
     case 'm' : setSquare(i*10+j,SILVER | CAMEL_PIECE);  break;
     case 'h' : setSquare(i*10+j,SILVER | HORSE_PIECE);  break;
     case 'd' : setSquare(i*10+j,SILVER | DOG_PIECE);   break;
     case 'c' : setSquare(i*10+j,SILVER | CAT_PIECE);   break;
     case 'r' : setSquare(i*10+j,SILVER | RABBIT_PIECE);  break;
     case ' ' : case 'X' : setSquare(i*10+j,EMPTY_SQUARE); break;
     default :
      log_() << "Unknown character " << c << " encountered while reading board at [" << i << "," << j << "].\n";
      f.close();
      return false;
      break;
    }
   }
   f.ignore(1024,'\n'); //finish the line
  }
  f.close();
 } 
 catch(int e) {
  return false; //initialization from file failed
 }
 return true;
 //BOARD_Calculate_Hashkey(bp);
}

void Board::setSquare(coord_t coord, square_t value) 
{
 board_[coord] = value;
}

square_t Board::getSquare(coord_t coord) 
{
 return board_[coord];
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

 for (int i = 1; i < 9; i++) {
   log_() << 9-i << "| ";
   for (int j = 1; j < 9; j++) {
     switch(getSquare(i*10+j)) {
       case (GOLD | ELEPHANT_PIECE) : log_() << "E "; break;
       case (GOLD | CAMEL_PIECE) : log_() << "M "; break;
       case (GOLD | HORSE_PIECE) : log_() << "H "; break;
       case (GOLD | DOG_PIECE) : log_() << "D "; break;
       case (GOLD | CAT_PIECE) : log_() << "C "; break;
       case (GOLD | RABBIT_PIECE) : log_() << "R "; break;

       case (SILVER | ELEPHANT_PIECE) : log_() << "e "; break;
       case (SILVER | CAMEL_PIECE) : log_() << "m "; break;
       case (SILVER | HORSE_PIECE) : log_() << "h "; break;
       case (SILVER | DOG_PIECE) : log_() << "d "; break;
       case (SILVER | CAT_PIECE) : log_() << "c "; break;
       case (SILVER | RABBIT_PIECE) : log_() << "r "; break;

       case EMPTY_SQUARE :
         if ((i==3 || i==6) && (j==3 || j==6))
           log_() << "- ";
         else
           log_() << ". ";
         break;
       default :
         log_() << "? ";
         break;
     }
   }
   log_() << "|" << endl;
 }

 log_() << " +-----------------+" << endl;
 log_() << "   a b c d e f g h" << endl;
 //log_() << "Hashkey: %#.8X%.8X\n",(unsigned long int)(bp->hashkey>>32),(unsigned long int)(bp->hashkey&0xFFFFFFFFULL);
} //Board::dump


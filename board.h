#ifndef __BOARD_H__
#define __BOARD_H__

#include "utils.h"

#define EMPTY_SQUARE 0x0U
#define EMPTY 0x0U
#define OFF_BOARD_SQUARE 0x9FU
#define OFF_BOARD 0x18U
#define GOLD 0x10U
#define SILVER 0x8U
#define OFF_BOARD_PIECE 0x7U
#define ELEPHANT_PIECE 0x6U
#define CAMEL_PIECE 0x5U
#define HORSE_PIECE 0x4U
#define DOG_PIECE 0x3U
#define CAT_PIECE 0x2U
#define RABBIT_PIECE 0x1U
#define EMPTY_PIECE 0x0U
#define PIECE_MASK 0x7U
#define OWNER_MASK 0x18U
#define FLIP_SIDE GOLD^SILVER
#define TRUE 1
#define FALSE 0
#define NORTH -10
#define SOUTH 10
#define EAST 1
#define WEST -1

typedef unsigned char square_t;
typedef unsigned int coord_t;

class Board
{
		unsigned char board_[100];	
		unsigned int  moveCnt_;
		unsigned int  toMove_;
		Logger        log_; 

	public:
		bool init(const char* fn); 

		bool isEmpty();
		bool isGoldMove();

		inline void setSquare(coord_t, square_t);
		inline square_t getSquare(coord_t);

		void dump();

		
};

#endif

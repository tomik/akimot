#ifndef BOARDH_DEFINED // Don't include this twice

#define BOARDH_DEFINED

// #define TESTING // define this to get a version suitable to run automatic tests with.

#define MAX_NUMBER_MOVES 100

// constants used for the board

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

// constants defining the various squares of the board

#define A1 81
#define A2 71
#define A3 61
#define A4 51
#define A5 41
#define A6 31
#define A7 21
#define A8 11
#define B1 82
#define B2 72
#define B3 62
#define B4 52
#define B5 42
#define B6 32
#define B7 22
#define B8 12
#define C1 83
#define C2 73
#define C3 63
#define C4 53
#define C5 43
#define C6 33
#define C7 23
#define C8 13
#define D1 84
#define D2 74
#define D3 64
#define D4 54
#define D5 44
#define D6 34
#define D7 24
#define D8 14
#define E1 85
#define E2 75
#define E3 65
#define E4 55
#define E5 45
#define E6 35
#define E7 25
#define E8 15
#define F1 86
#define F2 76
#define F3 66
#define F4 56
#define F5 46
#define F6 36
#define F7 26
#define F8 16
#define G1 87
#define G2 77
#define G3 67
#define G4 57
#define G5 47
#define G6 37
#define G7 27
#define G8 17
#define H1 88
#define H2 78
#define H3 68
#define H4 58
#define H5 48
#define H6 38
#define H7 28
#define H8 18

// macros for board manipulation

#define OWNER(square) (bp->board[square] & OWNER_MASK) // who owns a (piece on a) square?
#define PIECE(square) (bp->board[square] & PIECE_MASK) // what piece is on a square?
#define BOARD(square) (bp->board[square]) // What is on a square?  Returns the owner | piece combination.
#define ROW(square) (9-square/10) // what row is a square in?  1 = bottom, 8 = top
#define COL(square) (square%10) // what column is a square in?  1 = left (a), 8 = right (h)
#define PRINT_SQUARE(square) sprintf(message,"%c%c",COL(square)-1+'a',ROW(square)-1+'1'); BOARD_Message()

typedef struct
{
    unsigned char board[100]; 
        /*****
        11 - 88 = actual board, edges around for easier move generation and evaluation
        
        11 12 ... 17 18    a8 b8 ... g8 h8
        21 22 ... 27 28    a7 b7 ... g7 h7
        ............... == ...............
        71 72 ... 77 78    a2 b2 ... g2 h2
        81 82 ... 87 88    a1 b1 ... g1 h1
                
        directions:
            -10 : North (up, towards silver)
            +10 : South (down, towards gold)
            +1 : East (right from gold's view, left from silver's view)
            -1 : West (left from gold's view, right from silver's view)
            
        highest bit - is square off the board?
            (board[x]&0x80U)==0 : square is on board.
            (board[x]&0x80U)==0x80U : square is off the board.
        second, third bit - currently unused.
        fourth, fifth bit - who owns the piece?
            (board[x] & OWNER_MASK)==OFF_BOARD : square is off the board - both bits are set
            (board[x] & OWNER_MASK)==GOLD : gold piece - first bit is set
            (board[x] & OWNER_MASK)==SILVER : silver piece - second bit is set
            (board[x] & OWNER_MASK)==EMPTY : empty square - none of the bits are set
        remaining three bits - which kind of piece is it?
            (board[x]&0x7U) gives which piece it is.
                (board[x] & PIECE_MASK)==6 : Elephant
                (board[x] & PIECE_MASK)==5 : Camel
                (board[x] & PIECE_MASK)==4 : Horse
                (board[x] & PIECE_MASK)==3 : Dog
                (board[x] & PIECE_MASK)==2 : Cat
                (board[x] & PIECE_MASK)==1 : Rabbit
            Special cases:
                (board[x] & PIECE_MASK)==0 : Empty
                (board[x] & PIECE_MASK)==7 : Off the board
        *****/
    unsigned char at_move; // Who is at move?
    unsigned char steps; // How many steps have the side at move done so far?
    int move; // How many moves have been done in the game so far?  0 at start, 2 after setup... even means gold is at move, odd means silver is at move.  Divide by 2 and add 1 to get official move number.
    unsigned long long int hashkey; // 64-bit hashkey, used for index into hash table, and for collision / repetition detection
} board_t;

typedef struct
{
    unsigned char steps; // How many steps does this move use?
    unsigned char pass; // Are we passing?
    unsigned char piece[2]; // Which piece(s) are we moving?
    unsigned char from[2]; // Square piece(s) are moving from.
    unsigned char to[2]; // Square piece(s) are moving to.
    unsigned char capture_square[2]; // If a piece is captured, where does it happen?
    unsigned char captured_piece[2]; // If a piece is captured, which piece is it?
    unsigned long long int hashkey; // 64-bit hashkey, used for comparing moves
} move_t;

extern const int direction[4];
extern const int trap[4];
extern char message[1000];

void BOARD_Init(board_t *bp);
void BOARD_Calculate_Hashkey(board_t *bp);
void BOARD_Copy_Move(move_t *mpfrom, move_t *mpto);
void BOARD_Do_Move(board_t *bp, move_t *mp);
void BOARD_Undo_Move(board_t *bp, move_t *mp);
int BOARD_Read_Position(board_t *bp, char *file_name);
void BOARD_Print_Position(board_t *bp);
int BOARD_Generate_Moves(board_t *bp, move_t ml[MAX_NUMBER_MOVES]);
void BOARD_Print_Move(move_t *mp);
void BOARD_Send_Move(move_t *mp);
void BOARD_Message_Init(void);
void BOARD_Message(void);
void BOARD_Message_Exit(void);

#endif

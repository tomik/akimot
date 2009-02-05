#include "old_board.h"

const int direction[4]={OB_NORTH,OB_EAST,OB_SOUTH,OB_WEST};
const int rabbitForward[2]={OB_NORTH,OB_SOUTH};
const int rabbitWinRow[2]={8,1};
const int trap[4]={33,36,63,66};
u64 zobrist[2][7][100];     

//thirdRepetition class
ThirdRep*  OB_Board::thirdRep_;

// switch to know when to init static variables in class Board
bool OB_Board::classInit = false;

//---------------------------------------------------------------------
//  section PieceArray
//---------------------------------------------------------------------

PieceArray::PieceArray()
{
  len = 0;
}

//---------------------------------------------------------------------

void PieceArray::add(coord_t elem)
{
  elems[len++] = elem;
  assert(MAX_PIECES >= len);
}

//---------------------------------------------------------------------

void PieceArray::del(coord_t elem)
{
  for (uint i = 0; i < len;i++)
    if (elems[i] == elem){
      elems[i] = elems[--len];
      return;
    }
  assert(false);
}

//---------------------------------------------------------------------
 
void PieceArray::clear() 
{
  len = 0; 
}

//---------------------------------------------------------------------

string PieceArray::toString() const
{
  stringstream ss;
  ss.clear();

  for (uint i = 0; i < len; i++)
    ss << elems[i] << " ";  
  
  ss << endl;
  return ss.str();
}

//--------------------------------------------------------------------- 

uint PieceArray::getLen() const
{
  return len;
}

//---------------------------------------------------------------------
 
coord_t PieceArray::operator[](uint index) const
{
  assert( index >= 0 && index < len );
  return elems[index];
}

//--------------------------------------------------------------------- 

coord_t PieceArray::getRandom() const
{
  assert(len);
  return elems[rand() % len];
}

//---------------------------------------------------------------------
//  section Board
//---------------------------------------------------------------------
//

OB_Board::OB_Board()
{
}

//--------------------------------------------------------------------- 

void OB_Board::initNewGame()
{
  init(true);
}

//--------------------------------------------------------------------- 

bool OB_Board::initFromRecord(const char* fn)
{

  init();
  
  fstream f;
  string line;
  stringstream ss;
  string token;
  uint moveCount;

  try { 
    f.open(fn, fstream::in);
    if (! f.good()){
      f.close();
      return false;
    }

    
    while (f.good()){
      
      getline(f, line);
      ss.clear();
      ss.str(line);

      ss >> token; 
      moveCount = str2int(token.substr(0, token.length() - 1));
      assert(moveCount == moveCount_);
      makeMove(getStreamRest(ss));
    }

  }
  catch(int e) {
    return false;     //initialization from file failed
  }

  return true;

} 

//--------------------------------------------------------------------- 

bool OB_Board::operator== ( const OB_Board& board) const
{
  return (signature_ == board.signature_ && moveCount_ == board.moveCount_ ) ;
}

//---------------------------------------------------------------------

bool OB_Board::initFromPosition(const char* fn)
{
  fstream f;

  f.open(fn,fstream::in);

  if (! f.good()){
    f.close();
     return false;
   }

  bool result = initFromPositionStream(f);
  f.close();

  return result;
}

//---------------------------------------------------------------------

bool OB_Board::initFromPositionCompactString(const string& s)
{
  stringstream ss;
  ss.str(s);
  init();
  char side;
  ss >> side;

  toMove_ = sideCharToPlayer(side);
  if (! OB_IS_PLAYER(toMove_))
    return false;
  string boardStr = getStreamRest(ss);
  
  //check format '[' .. 64 characters for position ... ']'
  if (boardStr[0] != '[' || boardStr[65] != ']') 
    return false;

  uint k=1;
  for (int i = 8; i > 0; i--) 
    for (int j = 1; j < 9; j++) {
      assert(k < boardStr.length());
      char c = boardStr[k++];
      if (c == ' ' || c=='X' || c=='x')
        board_[i*10+j] = (OB_EMPTY_SQUARE); 
      else {
        try{
          PiecePair piecePair = parsePieceChar(c);
          board_[i*10+j] = (piecePair.first | piecePair.second);
        }catch (int e){
          logError("Unknown character %c encountered while reading board at [%d, %d]\n", c, i, j);
          return false;
        }
      }
    }

  afterPositionLoad();
  return true;
}

//--------------------------------------------------------------------- 

Step OB_Board::findMCstep() 
{
  Step step;

  //it's not possible to have 0 rabbits in the beginning of move
  assert( rabbitsNum[toMoveIndex_] > 0 || stepCount_ > 0); 

  //no piece for player to move 
  if (pieceArray[toMoveIndex_].getLen() == 0) {
    //step_pass since the player with no pieces still might win 
    //if he managed to kill opponent's last rabbit before he lost his last piece
    return Step(STEP_PASS, toMove_);          
  }

  if (! cfg.knowledgeInPlayout()){
    if (findRandomStep(step))
      return step;
  }

  stepArrayLen = 
      generateAllStepsNoPass(toMove_, stepArray);
  uint len = stepArrayLen;

  assert(stepArrayLen < MAX_STEPS);

  if (len == 0 ){ //player to move has no step to play - not even pass
    winner_ = OB_OPP(toMove_);
    return Step(STEP_NULL,toMove_); 
  }

  if (cfg.knowledgeInPlayout()){
    return chooseStepWithKnowledge(stepArray, stepArrayLen);
  }

  assert(len > 0);
  uint index = rand() % len;
  assert( index >= 0 && index < len );

  return( stepArray[index]); 
} 

//--------------------------------------------------------------------- 

void OB_Board::findMCmoveAndMake()
{
  //TODO area selection 
  
  if (pieceArray[toMoveIndex_].getLen() == 0){
    makeStepTryCommitMove(Step(STEP_PASS, toMove_));
    return;
  }

  Step step;
  PieceArray p;

  for (int i = 0; i < 2; i++){
    p.add(pieceArray[toMoveIndex_].getRandom());
  }

  do { 
    /*if (options.localMode()){
      cerr << toString();
      cerr << lastStep().toString();
    }
    */

    stepArrayLen = 1;
    stepArray[0] = Step(STEP_PASS, toMove_);

    for (uint i = 0; i < p.getLen(); i++) { 
      if (OB_OWNER(board_[p[i]]) == toMove_){
        generateStepsForPiece(p[i], stepArray, stepArrayLen);
      }
    }
    if (cfg.knowledgeInPlayout()){
      step = chooseStepWithKnowledge(stepArray, stepArrayLen);
    }
    else{
      step = stepArray[rand() % stepArrayLen];
    }
    if (! step.isPass()){
      p.del(step.from_);
      p.add(step.to_);
    }
  } while ( ! makeStepTryCommitMove(step));

  /*
  do {
    step = findMCstep();
  } while (! makeStepTryCommitMove(step));
  */

}

//--------------------------------------------------------------------- 

void OB_Board::getHeuristics(const StepArray& steps, uint stepsNum, HeurArray& heurs) const
{
  for (uint i = 0; i < stepsNum; i++){
    heurs[i] = evaluateStep(steps[i]); 
    //assert(int(heurs[i]) == heurs[i]);
  }
}

//--------------------------------------------------------------------- 

void OB_Board::init(bool newGame)
{

  stepArrayLen = 0;

  assert(OB_OPP(OB_GOLD) == OB_SILVER);
  assert(OB_OPP(OB_SILVER) == OB_GOLD);

  //game initialization - done in the first run or when newgame is specified
  if (! classInit || newGame) {
    classInit = true;
    initZobrist();  
    thirdRep.clear();
    thirdRep_ = &thirdRep;
  }

  for (int i = 0; i < 100; i++)  
    board_[i] = OB_OFF_BOARD_SQUARE;

  for (int i = 1; i < 9; i++)
    for (int j = 1; j < 9; j++){
      board_[10*i+j]       = OB_EMPTY_SQUARE;
      //frozenBoard is not in use now
      //frozenBoard_[10*i+j] = false;           //implicitly we consider everything not frozen
    }

  toMove_    = OB_GOLD;
  toMoveIndex_ = PLAYER_TO_INDEX(toMove_);
  stepCount_ = 0;
  moveCount_ = 1;
  lastStep_  = Step();
  winner_    = OB_EMPTY;

  //init pieceArray and rabbitsNum
  pieceArray[0].clear();
  pieceArray[1].clear();
  rabbitsNum[0] = 0;
  rabbitsNum[1] = 0;

  assert(PLAYER_TO_INDEX(OB_GOLD) == 0 && PLAYER_TO_INDEX(OB_SILVER) == 1);

  signature_ = 0;
  preMoveSignature_ = 0; 
  //signature is generated after the position is loaded
}

//---------------------------------------------------------------------


bool OB_Board::initFromPositionStream(istream& is)
{

  init();

  char side;
  char c;
  PiecePair piecePair;
  
  try { 

    is >> moveCount_; 
    is >> side;
    is.ignore(1024,'\n'); //ignores the rest of initial line 
    toMove_ = sideCharToPlayer(side);
    if (! OB_IS_PLAYER(toMove_))
      return false;
    toMoveIndex_ = PLAYER_TO_INDEX(toMove_);
    is.ignore(1024,'\n'); //ignores the top of the border till EOF 

    for (int i = 8; i > 0; i--) { // do this for each of the 8 lines of the board
      is.ignore(2); //ignore trailing characters

      for (int j = 1; j < 9; j++) {
        is.ignore(1); //ignore a white space 
        c=is.get();

        if (c == ' ' || c=='X' || c=='x' || c == '.')
          board_[i*10+j] = (OB_EMPTY_SQUARE); 
        else {
          try{
            piecePair = parsePieceChar(c);
            board_[i*10+j] = (piecePair.first | piecePair.second);
          }catch (int e){
            logError("Unknown character %c encountered while reading board at [%d, %d]\n", c, i, j);
            return false;
          }
        } 
      } 
      is.ignore(1024,'\n'); //finish the line
    } 
  } 
  catch(int e) {
    return false; //initialization from file failed
  }

  afterPositionLoad();

  return true;
}

//--------------------------------------------------------------------- 

ob_player_t OB_Board::sideCharToPlayer(char side) const
{
  ob_player_t player;
  if (! (side == 'w' || side == 'b' || side == 'g' || side =='s'))
    return OB_EMPTY;


  if (side == 'w' || side == 'g')
    player = OB_GOLD;
  else {
    player = OB_SILVER;
  }

  return player;
}

//--------------------------------------------------------------------- 

void OB_Board::afterPositionLoad()
{
  makeSignature();    //(unique) position identification
  preMoveSignature_ = signature_;

  for (int square = 11; square < 89; square++){
    if (OB_IS_PLAYER(board_[square]))
      pieceArray[PLAYER_TO_INDEX(OB_OWNER(board_[square]))].add(square);
    if (OB_PIECE(board_[square]) == OB_PIECE_RABBIT)
      rabbitsNum[PLAYER_TO_INDEX(OB_OWNER(board_[square]))]++; 
  }
}

//--------------------------------------------------------------------- 

recordAction_e  OB_Board::parseRecordActionToken(const string& token, ob_player_t& player, 
                                              ob_piece_t& piece, ob_square_t& from, ob_square_t& to)
{
  assert(token.length() == 3 || token.length() == 4);
  recordAction_e recordAction = ACTION_STEP;

  if (token.length() == 3) 
    recordAction = ACTION_PLACEMENT;
  else if (token[3] == 'x') //kill in trap
    recordAction = ACTION_TRAP_FALL; 

  //parse player/piece
  PiecePair piecePair = parsePieceChar(token[0]);
  player = piecePair.first;
  piece = piecePair.second;

  //parse from
  string columnRefStr("abcdefgh");
  string::size_type col = columnRefStr.find(token[1], 0);
  assert(col != string::npos);
  uint row = token[2] - '0';
  assert(row > 0 && row < 9);
  from = (row) * 10 + (col + 1);

  if (recordAction == ACTION_STEP){
    ob_square_t direction = 0; 
    switch  (token[3]){
      case 'n': direction = OB_NORTH;  
        break;  
      case 's': direction = OB_SOUTH;  
        break;  
      case 'e': direction = OB_EAST;  
        break;  
      case 'w': direction = OB_WEST;  
        break;  
      default:
        assert(false);
        break;
    }
    to = from + direction;
  }

  return recordAction;
}

//--------------------------------------------------------------------- 

PiecePair OB_Board::parsePieceChar(char pieceChar) 
{
  ob_player_t player;
  ob_piece_t piece;

  player = OB_GOLD;
  if (islower(pieceChar))
    player = OB_SILVER;
  pieceChar = tolower(pieceChar); 

  switch(pieceChar) {
    case 'e' : piece = OB_PIECE_ELEPHANT; break;
    case 'm' : piece = OB_PIECE_CAMEL;    break;
    case 'h' : piece = OB_PIECE_HORSE;    break;
    case 'd' : piece = OB_PIECE_DOG;      break;
    case 'c' : piece = OB_PIECE_CAT;      break;
    case 'r' : piece = OB_PIECE_RABBIT;   break;
    default:
      throw "Incorrect piece Character encountered.";
      break;
  }
  return PiecePair(player, piece); 
}


//---------------------------------------------------------------------

void OB_Board::initZobrist() const
{
   for (int i = 0; i < PLAYER_NUM; i++)
    for (int j = 0; j < OB_PIECE_NUM; j++)
      for (int k = 0; k < OB_SQUARE_NUM; k++){
        //cerr << k <<  " ~ "  << SQUARE_TO_INDEX_64(k) << endl;
        zobrist[i][j][k] = bits::zobrist[i][j][SQUARE_TO_INDEX_64(k)];
      }
  //exit(1);
}

//---------------------------------------------------------------------

void OB_Board::makeSignature()
{
  signature_ = 0;
  for (int i = 0; i < 100; i++)  
    if OB_IS_PLAYER(board_[i])
      signature_ ^= zobrist[PLAYER_TO_INDEX(OB_OWNER(board_[i]))][OB_PIECE(board_[i])][i] ;
}

//---------------------------------------------------------------------

void OB_Board::makeStep(const Step& step)
{
  
  lastStep_ = step;

  int playerIndex = PLAYER_TO_INDEX(step.player_);
  if (step.stepType_ == STEP_NULL)
    return;

  if (step.stepType_ == STEP_PASS ){
    stepCount_++; 
    return;
  }

  assert(step.stepType_ == STEP_PUSH || step.stepType_ == STEP_PULL || step.stepType_ == STEP_SINGLE );

  //in STEP_PUSH victim's move must be made first ! 
  if (step.stepType_ == STEP_PUSH ){  
    assert( stepCount_ < 3 ); 
    setSquare( step.oppTo_, OB_OPP(step.player_), step.oppPiece_);
    clearSquare(step.oppFrom_);
    stepCount_++;
    pieceArray[1 - playerIndex].del(step.oppFrom_);
    pieceArray[1 - playerIndex].add(step.oppTo_);

    checkKill(step.oppFrom_);
  }

  //single step
  setSquare( step.to_, step.player_, step.piece_);
  clearSquare(step.from_);
  stepCount_++;
  pieceArray[playerIndex].del(step.from_);
  pieceArray[playerIndex].add(step.to_);

  checkKill(step.from_);

  //pull steps
  if (step.stepType_ == STEP_PULL) {  
    assert( stepCount_ < 4 ); 
    setSquare( step.oppTo_, OB_OPP(step.player_), step.oppPiece_);
    clearSquare(step.oppFrom_);
    stepCount_++;
    pieceArray[1 - playerIndex].del(step.oppFrom_);
    pieceArray[1 - playerIndex].add(step.oppTo_);

    checkKill(step.oppFrom_);
  }

}

//---------------------------------------------------------------------

bool OB_Board::findRandomStep(Step& step) const
{
  
  bool found = false; //once set to true, move is generated and returned 
  
  bool tryLocal = false;
  if (cfg.localPlayout() && 
      lastStep_.player_ == int(toMove_) && 
      lastStep_.stepType_ != STEP_NULL && 
      OB_IS_PLAYER(board_[lastStep_.to_])){
    tryLocal = true;
  }

  for ( int i = 0; i < 5; i++){ 
    assert(pieceArray[toMoveIndex_].getLen() != 0);

    if (tryLocal and random01() > 0.2){
      tryLocal = false;
      step.from_ = lastStep_.to_;
    } 
    else{
      step.from_ = pieceArray[toMoveIndex_][rand() % pieceArray[toMoveIndex_].getLen()];
    }

    assert(OB_IS_PLAYER(board_[step.from_]));
    assert(OB_OWNER(board_[step.from_]) == toMove_);

    if (isFrozen(step.from_))
      continue;
   
    step.to_ = step.from_ + direction[rand() % 4];
    if ( board_[step.to_] == OB_EMPTY_SQUARE ){ //single/pull
      if (OB_PIECE(board_[step.from_]) == OB_PIECE_RABBIT &&   //rabbits cannot backwards
         (step.to_ - step.from_ == (toMove_ == OB_GOLD ? OB_SOUTH : OB_NORTH ))) {
        continue;
      }

      for (int j = 0; j < 4; j++){
        step.oppFrom_ = step.from_ + direction[j];    //todo - exclude "to" direction
        if ( OB_OWNER(board_[step.oppFrom_]) == OB_OPP(OB_OWNER(board_[step.from_])) &&
             OB_PIECE(board_[step.oppFrom_]) <  OB_PIECE(board_[step.from_])){
          found = true; //generate the step 
          step.stepType_ = STEP_PULL;
          assert(OB_OWNER(board_[step.oppFrom_]) == OB_OPP(toMove_)); 
          step.oppTo_ = step.from_; 
          break;
        }
      } 
      if (! found){
        step.stepType_ = STEP_SINGLE;
        found = true;
      }
    }else{ //push

      if ( OB_OWNER(board_[step.to_]) == OB_OPP(OB_OWNER(board_[step.from_])) &&
           OB_PIECE(board_[step.to_]) <  OB_PIECE(board_[step.from_])){
        assert(OB_IS_PLAYER(board_[step.to_]));
        for (int j = 0; j < 4; j++){
          if (board_[step.to_ + direction[j]] == OB_EMPTY_SQUARE){  //generate the step
            step.oppTo_ = step.to_ + direction[j];    
            step.stepType_ = STEP_PUSH;
            step.oppFrom_ = step.to_; 
            assert(OB_OWNER(board_[step.oppFrom_]) == OB_OPP(toMove_)); 
            found = true;
            break;
          }
        }
      }
    }

    if (found){
      step.player_ = toMove_;
      step.piece_  = OB_PIECE(board_[step.from_]);
      assert(OB_OWNER(board_[step.from_]) == toMove_);
      assert(step.stepType_ == STEP_SINGLE || step.stepType_ == STEP_PUSH || step.stepType_ == STEP_PULL);

      if (step.stepType_ != STEP_SINGLE) {
        //push/pull not allowed 
        if (stepCount_ > 2){
          found = false;
          continue;
        }
        assert(OB_OWNER(board_[step.oppFrom_]) == OB_OPP(toMove_)); 
        step.oppPiece_  = OB_PIECE(board_[step.oppFrom_]);
      }

      return true;
    }
  }
    return false;
}

//--------------------------------------------------------------------- 

Step OB_Board::chooseStepWithKnowledge(StepArray& steps, uint stepsNum) const
{
  assert(stepsNum > 0);
  uint bestIndex = stepsNum - 1;
  float bestEval = INT_MIN; 
  float eval = 0;
  //int r = smallRandomPrime();
  //int index;

  //cerr << toString();
  if ( cfg.knowledgeTournamentSize() == 0){
    for (uint i = 0; i < stepsNum; i++){
      //index = ((i+1)*r) % stepsNum;
      if (random01() >= 0.5)
        continue;
      const Step& step = steps[i];
      eval = evaluateStep(step); 
      //cerr << i << "/" << stepsNum << "/" << endl;
      cerr << step.toString() << " " << eval << " | ";
      if (eval > bestEval){
        bestEval = eval;
        bestIndex = i;
      }
    }
    cerr << steps[bestIndex].toString() << endl;
  }
  else {

    for (uint i = 0; i < (stepsNum/2 > cfg.knowledgeTournamentSize() ? cfg.knowledgeTournamentSize() : stepsNum/2); i++){
    //for (uint i = 0; i < cfg.knowledgeTournamentSize(); i++){
      uint r = rand() % stepsNum;
      assert(r >= 0 && r < stepsNum);
      const Step& step = steps[r];
      eval = evaluateStep(step); 
      if (eval > bestEval){
        bestEval = eval;
        bestIndex = r;
      }
    }
  }
  assert(bestIndex >= 0 && bestIndex < stepsNum);
  return steps[bestIndex];
}

//--------------------------------------------------------------------- 

float OB_Board::evaluateStep(const Step& step) const
{
  float eval = 0; 

  if (step.isPass()){
    eval -= 0.5 * (STEPS_IN_MOVE - stepCount_);
    return eval;
  }

  assert(step.pieceMoved());
  assert(OB_IS_PLAYER(board_[step.from_]));
  
  //we don't like inverse steps 
  //TODO ... inverse step is reasonable when something dies in the trap
  if (step.inversed(lastStep_)){
    eval -= 5;
    return eval;
  }

  if (step.piece_ == OB_PIECE_ELEPHANT ) {
    eval += 0.1;
  }

  if (step.isPushPull()){
    //push opponent to the goal :( not impossible ? )
    if (step.oppPiece_ == OB_PIECE_RABBIT && 
        ROW(step.oppTo_) == rabbitWinRow[PLAYER_TO_INDEX(OB_OPP(step.player_))]){
      eval -= 10;
    }
    //otherwise push/pulls are encouraged
    else{
      eval += 0.5; 
    }
  } 
  
  //check self-kill
  if (checkKillForward(step.from_, step.to_)){
    //allow self-kills to allow rabbits move to the goal
    //in the opponent's part of the board
    if (step.piece_ == OB_PIECE_RABBIT && ! OB_IS_TRAP(step.to_) && 
        ((toMove_ == OB_GOLD && ROW(step.from_) >= 4) ||
        (toMove_ == OB_SILVER && ROW(step.from_) <= 5))){
      eval -= 1;
    }
    else{
      eval -= 5;   
    }
  }
  else{

    //push opp to trap is good 
    if (step.isPushPull() && OB_IS_TRAP(step.oppTo_)){
      eval += 3;
    }

    //check opp-kill
    if (step.isPushPull() && checkKillForward(step.oppFrom_, step.oppTo_)){
      eval += 5;   
    }
  }

  //rabbit movements 
  if (step.piece_ == OB_PIECE_RABBIT){
    //moves in opponent's part of the board are encouraged
    if ((toMove_ == OB_GOLD && ROW(step.from_) >= 4) ||
        (toMove_ == OB_SILVER && ROW(step.from_) <= 5)){
      //empty space ahead is good 
      bool empty = true;
      int emptyNum = 0;
      int row = ROW(step.from_);
      int toEdge = toMove_ == OB_GOLD ? 
          TOP_ROW - row : 
          row - BOTTOM_ROW;
      while (emptyNum < toEdge){
        row += toMove_ == OB_GOLD ? +1 : -1 ;
        if (board_[row * 10 + COL(step.from_)] != OB_EMPTY_SQUARE){
          empty = false;
          assert(toEdge > emptyNum);
          break;
        }
        emptyNum++;
      } 
      //moving forward
      if (step.to_ - step.from_ == rabbitForward[toMoveIndex_]) {
        if (empty){
          eval += 5;
        }
        eval += emptyNum * 1;
      }
      //TODO add evaluation for moves left right

    }
    else { //move in player's part
      eval += -2;
    }
  } //rabbits

  //locality 
  if (cfg.localPlayout() && 
      lastStep_.stepType_ != STEP_NULL){
    int d = OB_SQUARE_DISTANCE(lastStep_.to_, step.from_);
    eval += d <= 3 ? (3 - d) * 0.5 : 0;
  }

  return eval;
}

//--------------------------------------------------------------------- 

bool OB_Board::checkKillForward(ob_square_t from, ob_square_t to, KillInfo* killInfo) const
{

  if ( OB_IS_TRAP(to) ) { //piece steps into the trap ( or is pushed/pulled in there ) 
    if ( ! hasTwoFriends(to, OB_OWNER(board_[from])) ) { 
      if (killInfo != NULL){
        killInfo->setValues(OB_OWNER(board_[from]), OB_PIECE(board_[from]), to); //activates as well
      }
      return true;
    }
    else //piece was moved into the trap but hasn't died -> no kill
      return false;
  }

  int actTrapPos;
  for (int i = 0; i < 4; i++){
    actTrapPos = from + direction[i];
    if ( OB_IS_TRAP(actTrapPos) && 
        board_[actTrapPos] != OB_EMPTY_SQUARE &&          //it is a trap where piece is standing
        OB_OWNER(board_[actTrapPos]) == OB_OWNER(board_[from]) && 
        ! hasTwoFriends(actTrapPos, OB_OWNER(board_[actTrapPos])) ){   //piece has < 2 friends
        if (killInfo != NULL){
          killInfo->setValues(OB_OWNER(board_[actTrapPos]), OB_PIECE(board_[actTrapPos]), actTrapPos);                   
        }
        return true;
    }
  }
  return false;
}

//--------------------------------------------------------------------- 

bool OB_Board::checkKill(ob_square_t square) 
{
  int actTrapPos;
  for (int i = 0; i < 4; i++){
    actTrapPos = square + direction[i];
    if ( OB_IS_TRAP(actTrapPos) ){    
      if ( board_[actTrapPos] != OB_EMPTY_SQUARE && ! hasFriend(actTrapPos) ){
        performKill(actTrapPos);
        return true;
      }
      return false;
    }
  }
  return false;
}

//--------------------------------------------------------------------- 

void OB_Board::performKill(ob_square_t trapPos)
{
  int playerIndex = PLAYER_TO_INDEX(OB_OWNER(board_[trapPos]));

  if (OB_PIECE(board_[trapPos]) == OB_PIECE_RABBIT)   //update rabbitsNum - for gameEnd check 
     rabbitsNum[playerIndex]--;
  clearSquare(trapPos);
  pieceArray[playerIndex].del(trapPos);
}

//---------------------------------------------------------------------

bool OB_Board::makeStepTryCommitMove(const Step& step) 
{
  makeStep(step);
	if (stepCount_ >= 4 || ! step.pieceMoved()) {
    updateWinner();
    commitMove();
    return true;
  }
  return false;
}

//---------------------------------------------------------------------

void OB_Board::makeMove(const string& moveRaw)
{
  string move = trimLeft(trimRight(moveRaw));
  if (move == "")
    return; 

  stringstream ss(move);

  while (ss.good()){
    recordAction_e recordAction;
    string token;
    ob_player_t player; 
    ob_piece_t  piece;
    ob_square_t from;
    ob_square_t to;

    ss >> token;

    recordAction = parseRecordActionToken(token, player, piece, from, to);
    Step step = Step(STEP_SINGLE, player, piece, from, to);

    switch (recordAction){
      case ACTION_PLACEMENT:
        assert(moveCount_ == 1);
        setSquare(from, player, piece);
        pieceArray[PLAYER_TO_INDEX(player)].add(from);
        if (piece == OB_PIECE_RABBIT)
          rabbitsNum[PLAYER_TO_INDEX(player)]++; 
        break;
      case ACTION_STEP:
        assert(moveCount_ > 1);
        makeStep(step);
        break;
      case ACTION_TRAP_FALL:
      case ACTION_ERROR:
        break;
    }
  }

  commitMove();
}


//---------------------------------------------------------------------

void OB_Board::makeMove(const Move& move)
{
  makeMoveNoCommit(move);
  commitMove();
}

//---------------------------------------------------------------------

void OB_Board::makeMoveNoCommit(const Move& move)
{
  StepList stepList;
  stepList  = move.getStepList();

  assert(stepList.size() <= STEPS_IN_MOVE);
  for (StepListIter it = stepList.begin(); it != stepList.end(); it++)
    makeStep(*it);

}

//--------------------------------------------------------------------- 

void OB_Board::commitMove()
{
  assert(toMove_ == OB_GOLD || toMove_ == OB_SILVER);
  if (toMove_ == OB_SILVER) 
    moveCount_++;
  toMove_ = OB_OPP(toMove_);
  toMoveIndex_ = 1 - toMoveIndex_;
  assert(toMoveIndex_ == uint(PLAYER_TO_INDEX(toMove_)));
  stepCount_ = 0;

  //preMoveSignature of the next move is the actual signature
  preMoveSignature_ = signature_;
}


//--------------------------------------------------------------------- 

void OB_Board::updateWinner()
{
  //assert(winner_ == OB_EMPTY);
  //winner might be set already ( when player to move has no move ) 
  for (int i = 11; i <= 19; i++)
    if (board_[i] == (OB_SILVER | OB_PIECE_RABBIT) )
      winner_ = OB_SILVER;
  for (int i = 81; i <= 88; i++)
    if (board_[i] == (OB_GOLD | OB_PIECE_RABBIT) )
      winner_ = OB_GOLD;

  if (rabbitsNum[toMoveIndex_] <= 0)  //if player lost his last rabbit in his move - he loses ... 
    winner_ = OB_OPP(toMove_);
  if (rabbitsNum[1 - toMoveIndex_] <= 0)  //unless the other also lost his last rabbit 
    winner_ = toMove_;
}

//--------------------------------------------------------------------- 


bool OB_Board::quickGoalCheck(ob_player_t player, int stepLimit, Move* move) const
{
  assert(OB_IS_PLAYER(player));
  assert(stepLimit <= STEPS_IN_MOVE);

  queue<int> q;
  FlagBoard flagBoard;
  int rabbitDirection[3] = {player == OB_GOLD ? OB_NORTH : OB_SOUTH, OB_EAST, OB_WEST}; 
  int winRow[2] = {TOP_ROW, BOTTOM_ROW};
  int index = PLAYER_TO_INDEX(player);

  //bounds for flagBoard - depends on player
  //only part of flagBoard is "interesting"
  //conditions on rabbits should hold search in these bounds 
  //we get like 10% speedup in quickGoalCheck by this
  //TODO this might be still narrowed when stepLimit < 4
  const int lower = player == OB_GOLD ? 41 : 11;
  const int upper = player == OB_GOLD ? 89 : 59;

  //init with rabbits having chance to reach the goal (distance 4 from some trap)
  for (uint i = 0; i < pieceArray[index].getLen(); i++){
    if (OB_PIECE(board_[pieceArray[index][i]]) != OB_PIECE_RABBIT || 
        abs(ROW(pieceArray[index][i]) - winRow[index]) > stepLimit){
      continue;
    }

    //narrower (=>quicker) than [0, OB_SQUARE_NUM])
    for (int k = lower; k < upper; k++ ){
      flagBoard[k] = FLAG_BOARD_EMPTY;
    } 
    
    //out of board
    uint sacrificeTrap = 0;
    //sacrifice trap issues
    for (int j = 0; j < 4; j++){
      int ngb = pieceArray[index][i] + direction[j];
      if ( OB_IS_TRAP(ngb) && 
          OB_OWNER(board_[ngb]) == player && 
          ! hasTwoFriends(ngb, player)){
        assert(board_[ngb] != OB_EMPTY_SQUARE);
        sacrificeTrap = ngb;
      }
    }

    q.push(pieceArray[index][i]);
    flagBoard[pieceArray[index][i]] = 0;
   
    bool lastValid = true;
    int last = 0;
    //wave
    while(! q.empty()){
      int act = q.front();
      q.pop(); 
      if (! lastValid)
        flagBoard[last] = FLAG_BOARD_EMPTY;
      lastValid = false;
      last = act;
      //too many steps
      if (flagBoard[act] >= stepLimit) 
        continue;
      //too faw away to reach the goal/TODO - more elaborate check ? :)
      if (abs(ROW(act) - winRow[index]) > (stepLimit - flagBoard[act]))
        continue;
      //frozen or in the trap
      if (flagBoard[act] == 1  || 
          (flagBoard[act] == 2 && OB_SQUARE_DISTANCE(act, sacrificeTrap) == 1)) {
        if (!hasTwoFriends(act, player) && 
            (hasStrongerEnemy(act, player, OB_PIECE_RABBIT) || OB_IS_TRAP(act)))
          continue;
      }
      else{
        if (!hasFriend(act, player) && 
            (hasStrongerEnemy(act, player, OB_PIECE_RABBIT) || OB_IS_TRAP(act)))
          continue;
      }

      lastValid = true;
      //add neighbours 
      for (int l = 0; l < 3; l++){
        //neighbour
        int ngb = act + rabbitDirection[l];
        if (flagBoard[ngb] != FLAG_BOARD_EMPTY ) {
          continue; 
        }
        if ( board_[ngb] == OB_EMPTY_SQUARE ){ 
          if (ROW(ngb) == winRow[index]){
            if (move != NULL){
              flagBoard[ngb] = flagBoard[act] + 1;
              (*move) = tracebackFlagBoard(flagBoard, ngb, player);
            }
            return true;
          }
          q.push(ngb);
          flagBoard[ngb] = flagBoard[act] + 1;
        }
      }
    }
  }

  return false;
  
}

//--------------------------------------------------------------------- 

bool OB_Board::quickGoalCheck(Move* move) const
{
  return quickGoalCheck(toMove_, STEPS_IN_MOVE - stepCount_, move );
}

//---------------------------------------------------------------------

Move OB_Board::tracebackFlagBoard(const FlagBoard& flagBoard, 
                                int win_square, ob_player_t player) const
{
  assert(flagBoard[win_square] <= STEPS_IN_MOVE);
  assert(flagBoard[win_square] > 0);

  Move move;
  //inverse directions ... trackback
  int act = win_square;
  bool found_nbg;

  for (int i = 0; i < flagBoard[win_square]; i++) {
    found_nbg = false;
    for (int j = 0; j < 4; j++){
      int nbg = act + direction[j];
      if ( board_[nbg] != OB_OFF_BOARD_SQUARE &&
          flagBoard[nbg] == flagBoard[act] - 1) {
        //going backwards => prepending! 
        move.prependStep(Step(STEP_SINGLE, player, OB_PIECE_RABBIT, nbg, act));
        act = nbg;
        found_nbg = true;
        break;
      }
    }
    assert(found_nbg);
  }

  return move;
}

//---------------------------------------------------------------------

u64 OB_Board::calcAfterStepSignature(const Step& step) const
{
  if (step.stepType_ == STEP_PASS)
    return signature_;

  assert(step.stepType_ == STEP_PUSH || 
        step.stepType_ == STEP_PULL || 
        step.stepType_ == STEP_SINGLE );

  int checkTrap[2];         //squares in whose vicinity we check the traps
  int checkTrapNum = 0;

  u64 newSignature = signature_;

  if (step.stepType_ == STEP_PUSH || step.stepType_ == STEP_PULL){
    newSignature ^= zobrist[PLAYER_TO_INDEX(OB_OPP(toMove_))][step.oppPiece_][step.oppFrom_];  //erase previous location
    if ( (! OB_IS_TRAP(step.oppTo_)) || hasTwoFriends(step.oppTo_, OB_OPP(toMove_)) ){              //if not killed in the trap
      newSignature ^= zobrist[PLAYER_TO_INDEX(OB_OPP(toMove_))][step.oppPiece_][step.oppTo_];  //add new location 
      checkTrap[checkTrapNum++] = step.oppFrom_;
    }
  }

  newSignature ^= zobrist[PLAYER_TO_INDEX(toMove_)][step.piece_][step.from_];               //erase previous location 
  if ( ! OB_IS_TRAP(step.to_) || hasTwoFriends(step.to_, toMove_) ){                   //if not killed in the trap
    newSignature ^= zobrist[PLAYER_TO_INDEX(toMove_)][step.piece_][step.to_];               //add new location 
    checkTrap[checkTrapNum++] = step.from_;
  }
  
  //now check only for cases when moving a piece causes another (friendly) piece die in the trap because of lack of friends
  int actTrap;
  for (int j = 0; j < checkTrapNum; j++){
    for (int i = 0; i < 4; i++){
      actTrap = checkTrap[j] + direction[i];
      if ( OB_IS_TRAP(actTrap) &&                                          //there is a trap next to the position of piece
           board_[actTrap] != OB_EMPTY_SQUARE &&                           
           OB_OWNER(board_[actTrap]) == OB_OWNER(board_[checkTrap[j]]) &&    //there is a friend in the trap 
           ! hasTwoFriends(actTrap, OB_OWNER(board_[actTrap])) )          //he will die on lack of friends
        newSignature ^= zobrist[PLAYER_TO_INDEX(OB_OWNER(board_[actTrap]))][OB_PIECE(board_[actTrap])][actTrap];      //erase 
    } 
  }

  return newSignature;
}

//---------------------------------------------------------------------

int OB_Board::generateAllStepsNoPass(ob_player_t player, StepArray& steps) const
{
  uint stepsNum = 0;
  int square;

  //go through pieces of player 
  for (uint index =0 ; 
      index < pieceArray[PLAYER_TO_INDEX(player)].getLen(); index++) { 
    square = pieceArray[PLAYER_TO_INDEX(player)][index];
    assert(OB_OWNER(board_[square]) == player); 
    generateStepsForPiece(square, steps, stepsNum);
  } //for pieces on board

  return stepsNum;
}

//--------------------------------------------------------------------- 

int OB_Board::generateAllSteps(ob_player_t player, StepArray& steps) const {
  int stepsNum = generateAllStepsNoPass(player, steps);
  if (canPass()){
    steps[stepsNum++] = Step(STEP_PASS, player);
  }
  return stepsNum;
}

//--------------------------------------------------------------------- 

void OB_Board::generateStepsForPiece(
        ob_square_t square, StepArray& steps, uint& stepsNum) const { 

  if ( isFrozen(square))  //frozen
    return; 
  ob_player_t player = OB_OWNER(board_[square]);

  //generate push/pull moves
  if (stepCount_ < 3) {    
    for (int i = 0; i < 4; i++) {  
      if (OB_OWNER(board_[square + direction[i]]) == OB_OPP(player) 
          && OB_PIECE(board_[square + direction[i]]) < OB_PIECE(board_[square])){ //weaker enemy
        for (int j=0; j<4; j++)  // pull
          if (board_[square + direction[j]] == OB_EMPTY_SQUARE) { //create move
              steps[stepsNum++].setValues( STEP_PULL, player,OB_PIECE(board_[square]), square, 
                    square + direction[j], OB_PIECE(board_[square + direction[i]]), square + direction[i], square);
          }
        for (int j=0; j<4; j++)  //push
          if (board_[square + direction[i] + direction[j]] == OB_EMPTY_SQUARE) { //create move
              steps[stepsNum++].setValues( STEP_PUSH, player, OB_PIECE(board_[square]), square, 
                    square + direction[i], OB_PIECE(board_[square + direction[i]]),
                    square + direction[i], square + direction[i] + direction[j]);
        }
      } //if weaker enemy
    } //for
  } 

  // generate single moves
  for (int i=0; i<4; i++) // check each direction
    if (board_[square + direction[i]] == OB_EMPTY_SQUARE)  {
      if (OB_PIECE(board_[square]) == OB_PIECE_RABBIT){ // rabbit cannot backwards
        if (OB_OWNER(board_[square]) == OB_GOLD && direction[i] == OB_SOUTH)
          continue;
        if (OB_OWNER(board_[square]) == OB_SILVER && direction[i] == OB_NORTH)
          continue;
      }
      //create move
      steps[stepsNum++].setValues(STEP_SINGLE, player, OB_PIECE(board_[square]), 
                                      square, square + direction[i]);
    }

  
}

//---------------------------------------------------------------------

int OB_Board::filterRepetitions(StepArray& steps, int stepsNum) const 
{

  //check virtual passes ( immediate repetetitions ) 
  //these might be checked and pruned even if the move is not over yet 
  //e.g. (Eg5n) (Eg5s) is pruned 
  
  int i = 0;
  while (i < stepsNum) 
    if (stepIsVirtualPass(steps[i]))
      steps[i] = steps[stepsNum-- - 1];
    else
      i++;  

  //check third time repetitions
  //this can be checked only for steps that finish the move
  //these are : pass, step_single for stepCount == 3, push/pull for stepCount == 2 

  if ( stepCount_ < 2 ) 
    return stepsNum;

  i = 0;
  while (i < stepsNum)
    if ((stepCount_ == 3 || steps[i].isPushPull()) &&  stepIsThirdRepetition(steps[i]))
      steps[i] = steps[stepsNum-- - 1];
    else
      i++;  
  

  return stepsNum;
}

//---------------------------------------------------------------------

bool OB_Board::stepIsVirtualPass( Step& step ) const 
{
  u64 afterStepSignature = calcAfterStepSignature(step);
  if (afterStepSignature == preMoveSignature_) 
    return true;
  return false;
  
}

//---------------------------------------------------------------------

bool OB_Board::stepIsThirdRepetition(const Step& step ) const 
{
  u64 afterStepSignature = calcAfterStepSignature(step);
  assert(1 - PLAYER_TO_INDEX(step.getPlayer()) == 
        PLAYER_TO_INDEX(getPlayerToMoveAfterStep(step)));
  //check whether position with opponent to move won't be a repetition
  if ( thirdRep_->isThirdRep(afterStepSignature, 1 - PLAYER_TO_INDEX(step.getPlayer()))) 
    return true;
  return false;
  
}

//---------------------------------------------------------------------

bool OB_Board::hasFriend(ob_square_t square) const 
{ 
  uint owner = OB_OWNER(board_[square]);
  return hasFriend(square, owner);
}

//--------------------------------------------------------------------- 

bool OB_Board::hasFriend(ob_square_t square, ob_player_t owner) const 
{ 
  assert( owner == OB_GOLD || owner == OB_SILVER );
  for(int i = 0; i < 4; i++)
    if (OB_OWNER(board_[square + direction[i]]) == owner)
      return true;

  return false;
}

//---------------------------------------------------------------------

bool OB_Board::hasTwoFriends(ob_square_t square, ob_player_t player) const 
{ 
  assert( player == OB_GOLD || player == OB_SILVER );
  bool hasFriend = false;
  for(int i = 0; i < 4; i++)
    if (OB_OWNER(board_[square + direction[i]]) == player) {
      if (hasFriend)
        return true;
      hasFriend = true;
    }
  return false;
}

//---------------------------------------------------------------------

bool OB_Board::hasStrongerEnemy(ob_square_t square) const
{
  uint owner = OB_OWNER(board_[square]);
  return hasStrongerEnemy(square, owner, OB_PIECE(board_[square]));
}

//--------------------------------------------------------------------- 

bool OB_Board::hasStrongerEnemy(ob_square_t square, ob_player_t owner, ob_piece_t piece) const
{
  assert( owner == OB_GOLD || owner == OB_SILVER );
  for(int i = 0; i < 4; i++)
    if (OB_OWNER(board_[square + direction[i]]) == OB_OPP(owner) && 
        OB_PIECE(board_[square + direction[i]]) > piece)
      return true;

  return false;
  
}

//---------------------------------------------------------------------

bool OB_Board::isFrozen(ob_square_t square) const
{
  return (!hasFriend(square) && hasStrongerEnemy(square)); 
}

//---------------------------------------------------------------------

bool OB_Board::isSetupPhase() const
{
  return moveCount_ == 1 && pieceArray[toMoveIndex_].getLen() == 0; 
}

//---------------------------------------------------------------------

uint OB_Board::getStepCount() const
{
  return stepCount_;
}

//--------------------------------------------------------------------- 

u64 OB_Board::getSignature() const
{
  return signature_;
}

//---------------------------------------------------------------------

u64 OB_Board::getPreMoveSignature() const
{
  return preMoveSignature_; 
}

//---------------------------------------------------------------------

ob_player_t OB_Board::getPlayerToMove() const
{
  return toMove_;
}

//---------------------------------------------------------------------

ob_player_t OB_Board::getPlayerToMoveAfterStep(const Step& step) const
{ 
  //TODO what about resing step ? 
  assert( step.isPass() || step.isPushPull() || step.isSingleStep());
  ob_player_t player = toMove_;

  //check whether player switches after the step
  if (step.isPass() || stepCount_ == 3 || (stepCount_ == 2 && step.isPushPull())) 
    player = OB_OPP(player);
  return player;
}

//---------------------------------------------------------------------

ob_player_t OB_Board::getWinner() const
{
  return winner_;
}

//--------------------------------------------------------------------- 

ob_player_t OB_Board::gameOver() const
{
  return OB_IS_PLAYER(winner_);
}

//---------------------------------------------------------------------

bool OB_Board::canContinue(const Move& move) const
{
  return (getStepCount() + move.getStepCount() ) < STEPS_IN_MOVE;
}

//--------------------------------------------------------------------- 
  
bool OB_Board::canPass() const
{
  return stepCount_ > 0 && ! stepIsThirdRepetition(Step(STEP_PASS, toMove_));
}

//--------------------------------------------------------------------- 

Step OB_Board::lastStep() const 
{
  return lastStep_;
}

//--------------------------------------------------------------------- 

/*places piece at given position and updates signature*/
void OB_Board::setSquare( ob_square_t square, ob_player_t player, ob_piece_t piece)  
{
  signature_ ^=  zobrist[PLAYER_TO_INDEX(player)][piece][square]; 
  board_[square] = ( player | piece ); 
}

//---------------------------------------------------------------------

/*removes piece from given position and updates signature*/ 
void OB_Board::clearSquare( ob_square_t square) 
{
  signature_ ^=  zobrist[PLAYER_TO_INDEX(OB_OWNER(board_[square]))][OB_PIECE(board_[square])][square]; 
  board_[square] = OB_EMPTY_SQUARE; 
} 

//---------------------------------------------------------------------

string OB_Board::toString() const
{

  stringstream ss;

  
  ss << endl;

  //ss << pieceArray[0].toString() << pieceArray[1].toString();


  //print bits::zobrist
  /*
  for (int i = 0; i < PLAYER_NUM; i++)
   for (int j = 0; j < OB_PIECE_NUM; j++)
     for (int k = 0; k < OB_SQUARE_NUM; k++)
        ss << bits::zobrist[i][j][k] << endl;
  */


  //ss << "board at: " << this << endl; //pointer debug
  //#ifdef DEBUG_1
  ss << "Signature " << signature_ << endl;
  //#endif

  ss << "Move " << (toMove_ == OB_GOLD ? "g" : "s") << moveCount_ ;
  #ifdef DEBUG_2
    ss << ", Step " << stepCount_ << ", ";
  #endif 
  ss << endl;
    
  assert(toMove_ == OB_GOLD || toMove_ == OB_SILVER );

  ss << " +-----------------+\n";

  for (int i=8; i>0; i--) {
    ss << i <<"| ";
    for (int j=1; j<9; j++) {

      switch(board_[i*10+j]){
        case (OB_GOLD | OB_PIECE_ELEPHANT) :    ss << "E"; break;
        case (OB_GOLD | OB_PIECE_CAMEL) :       ss << "M"; break;
        case (OB_GOLD | OB_PIECE_HORSE) :       ss << "H"; break;
        case (OB_GOLD | OB_PIECE_DOG) :         ss << "D"; break;
        case (OB_GOLD | OB_PIECE_CAT) :         ss << "C"; break;
        case (OB_GOLD | OB_PIECE_RABBIT) :      ss << "R"; break;
        case (OB_SILVER | OB_PIECE_ELEPHANT) :    ss << "e"; break;
        case (OB_SILVER | OB_PIECE_CAMEL) :       ss << "m"; break;
        case (OB_SILVER | OB_PIECE_HORSE) :       ss << "h"; break;
        case (OB_SILVER | OB_PIECE_DOG) :         ss << "d"; break;
        case (OB_SILVER | OB_PIECE_CAT) :         ss << "c"; break;
        case (OB_SILVER | OB_PIECE_RABBIT) :      ss << "r"; break;
        case OB_EMPTY_SQUARE :
          if ((i==3 || i==6) && (j==3 || j==6))
              ss << "X ";
          else
              ss << ". ";
          break;
        default :
            ss << "? ";
            break;
      }
      if ( OB_OWNER(board_[i * 10 + j]) != OB_EMPTY){
        //assert( frozenOB_Board_[i * 10 + j] == isFrozen( i * 10 + j)); uncomment when ready
        if (frozenBoard_[i * 10 + j] )
          ss << "*";
        else
          ss << " ";
      }
    }
   ss << "| " << endl;
  }

  ss << " +-----------------+" << endl;
  ss << "   a b c d e f g h" << endl;

  return ss.str();
} //Board::dump

//---------------------------------------------------------------------

string OB_Board::MovetoStringWithKills(const Move& move) const
{
  OB_Board * playBoard = new OB_Board(*this);
  string s;
  StepList stepList = move.getStepList();
  for (StepListIter it = stepList.begin(); it != stepList.end(); it++){
    s = "TODO ! "; //s = s + StepWithKills((*it), playBoard).toString();
    playBoard->makeStepTryCommitMove(*it);
  }
  delete playBoard;
  return s;
}

//---------------------------------------------------------------------

string OB_Board::allStepsToString() const
{
  stringstream ss;

  for (uint playerIndex = 0; playerIndex < 2; playerIndex++) {
    if ( playerIndex ) 
      ss << endl << "Potential moves for OB_SILVER: " << endl;
    else
      ss << endl << "Potential moves for OB_GOLD: " << endl;

    /*for ( uint i = 0; i < stepArrayLen; i++){
      ss << stepArray[playerIndex][i].toString();
    }
    */
  }
  ss << endl;
  return ss.str();
}

//---------------------------------------------------------------------

void OB_Board::dump() const
{
  logRaw(toString().c_str());
}

//---------------------------------------------------------------------
// section OB_SimplePlayout
//---------------------------------------------------------------------

OB_SimplePlayout::OB_SimplePlayout()
{
  assert(false);
}

//---------------------------------------------------------------------

OB_SimplePlayout::OB_SimplePlayout(OB_Board* board, uint maxPlayoutLength, uint evalAfterLength):
  board_(board), playoutLength_(0), maxPlayoutLength_(maxPlayoutLength), evalAfterLength_(evalAfterLength)
{
  ;
}

//---------------------------------------------------------------------

void OB_SimplePlayout::doPlayout()
{
  uint moves = 0;

  while (true) {  
	  playOne();
	  playoutLength_++;
    if (hasWinner())
	    return ;

		if (playoutLength_ > 2 * maxPlayoutLength_) 
			return ;

    if (++moves >= evalAfterLength_ && evalAfterLength_)
      return ;
	}
}

//--------------------------------------------------------------------- 

bool OB_SimplePlayout::hasWinner(){
	if ( board_->getWinner() != OB_EMPTY ) {
    return true;  
  }
  return false;
}

//---------------------------------------------------------------------

void OB_SimplePlayout::playOne()
{
   //board_->findMCmoveAndMake();
	Step step;

	do {
		step = board_->findMCstep();
	}
	while (! board_->makeStepTryCommitMove(step));
	return;
}

//---------------------------------------------------------------------

uint OB_SimplePlayout::getPlayoutLength()
{
	return playoutLength_/2;
}

//---------------------------------------------------------------------
// 

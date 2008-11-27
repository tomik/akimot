#include "benchmark.h"

//---------------------------------------------------------------------
//  section Benchmark
//---------------------------------------------------------------------- 

Benchmark::Benchmark()
{
  board_ = new Board();
  board_->initNewGame(); 
  //TODO REPLACE LOADING FROM FILE WITH FUNCTION  
  board_->initFromPosition(START_POS_PATH);
  playoutCount_ = PLAYOUT_NUM;

}

//--------------------------------------------------------------------- 

Benchmark::Benchmark(Board* board, uint playoutCount)
{
	board_ = board;	
	playoutCount_ = playoutCount;
}

//--------------------------------------------------------------------- 

void Benchmark::benchmarkEval() const
{
  Eval eval; 
  clock_t clockBegin;
  float timeTotal;

  clockBegin = clock();

  for (uint i = 0 ; i < EVAL_NUM; i++)  
    eval.evaluate(board_);

	timeTotal = float (clock() - clockBegin) / CLOCKS_PER_SEC;

  logRaw("Evaluation performance: \n  %d evals\n  %3.2f seconds\n  %d eps\n", 
            EVAL_NUM, timeTotal, int ( float(EVAL_NUM) / timeTotal));
}

//--------------------------------------------------------------------- 

void Benchmark::benchmarkPlayout() const
{
		
  clock_t				clockBegin;
  float					timeTotal;

  playoutStatus_e  playoutStatus;
	uint			 playoutAvgLen = 0;

  clockBegin = clock();

  for (uint i = 0 ; i < playoutCount_; i++)  {

    //here we copy the given board
    Board *playBoard = new Board(*board_);

    SimplePlayout simplePlayout(playBoard, PLAYOUT_DEPTH, 0);
    playoutStatus = simplePlayout.doPlayout ();

		playoutAvgLen += simplePlayout.getPlayoutLength(); 
  }

	timeTotal = float (clock() - clockBegin) / CLOCKS_PER_SEC;
  logRaw("Playouts performance: \n  %d playouts\n  %3.2f seconds\n  %d pps\n  %d average playout length\n", 
            playoutCount_, timeTotal, int ( float(playoutCount_) / timeTotal),int(playoutAvgLen/ float (playoutCount_)));
  
}

//--------------------------------------------------------------------- 

void Benchmark::benchmarkUct() const
{
  //tree with random player in the root
  Tree* tree = new Tree(INDEX_TO_PLAYER(random() % 2));
  int winValue[2] = {1, -1};
  StepArray steps;
  int stepsNum;
  int i = 0;
  int walks = 0;

  float timeTotal;
  clock_t clockBegin;
 
  clockBegin = clock();

  stepsNum = UCT_CHILDREN_NUM;
  for (int i = 0; i < UCT_CHILDREN_NUM; i++)
    steps[i] = Step(STEP_PASS, GOLD);

  while (true) {
    walks++;
    if (i >= UCT_NODES_NUM)
      break;
    tree->historyReset();     //point tree's actNode to the root 
    while (true) { 
      if (! tree->actNode()->hasChildren()) { 
        if (tree->actNode()->getVisits() > UCT_NODE_MATURE) {
          tree->actNode()->expand(steps,stepsNum);
          i++;
          continue;
        }
        tree->updateHistory(winValue[(random() % 2)]);
        break;
      }
      tree->uctDescend(); 
    } 
  }

	timeTotal = float (clock() - clockBegin) / CLOCKS_PER_SEC;
  logRaw("Uct performance: \n  %d walks\n  %3.2f seconds\n  %d wps\n  %d nodes\n", 
            walks, timeTotal, int ( float(walks) / timeTotal), UCT_NODES_NUM);

}
//--------------------------------------------------------------------- 

void Benchmark::benchmarkAll() const
{
  benchmarkEval();
  benchmarkPlayout();
  benchmarkUct();
}

//--------------------------------------------------------------------- 
//--------------------------------------------------------------------- 


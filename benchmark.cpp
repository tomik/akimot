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
  timer.setTimer(SEC_ONE);
}

//--------------------------------------------------------------------- 

Benchmark::Benchmark(Board* board, uint playoutCount)
{
	board_ = board;	
}

//--------------------------------------------------------------------- 

void Benchmark::benchmarkEval() 
{
  Eval eval; 
  float timeTotal;

  timer.start();

  int i = 0;
  while (! timer.timeUp()){
    i++; 
    eval.evaluate(board_);
  }

  timer.stop();
	timeTotal = timer.elapsed(); 

  logRaw("Evaluation performance: \n  %d evals\n  %3.2f seconds\n  %d eps\n", 
            i, timeTotal, int ( float(i) / timeTotal));
}

//--------------------------------------------------------------------- 

void Benchmark::benchmarkPlayout() 
{
		
  float					timeTotal;

  playoutStatus_e  playoutStatus;
	uint			 playoutAvgLen = 0;

  timer.start();

  int i = 0;
  while (! timer.timeUp()){
    i++;
    //here we copy the given board
    Board *playBoard = new Board(*board_);

    SimplePlayout simplePlayout(playBoard, PLAYOUT_DEPTH, 0);
    playoutStatus = simplePlayout.doPlayout ();

		playoutAvgLen += simplePlayout.getPlayoutLength(); 
  }

  timer.stop();
	timeTotal = timer.elapsed(); 
  logRaw("Playouts performance: \n  %d playouts\n  %3.2f seconds\n  %d pps\n  %d average playout length\n", 
            i, timeTotal, int ( float(i) / timeTotal),int(playoutAvgLen/ float (i)));
  
}

//--------------------------------------------------------------------- 

void Benchmark::benchmarkUct() 
{
  //tree with random player in the root
  Tree* tree = new Tree(INDEX_TO_PLAYER(random() % 2));
  int winValue[2] = {1, -1};
  StepArray steps;
  int stepsNum;
  int i = 0;
  int walks = 0;

  float timeTotal;
 
  timer.start();

  stepsNum = UCT_CHILDREN_NUM;
  for (int i = 0; i < UCT_CHILDREN_NUM; i++)
    steps[i] = Step(STEP_PASS, GOLD);

  while (true) {
    walks++;
    if ( timer.timeUp() )
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

  timer.stop();
  timeTotal = timer.elapsed();
  logRaw("Uct performance: \n  %d walks\n  %3.2f seconds\n  %d wps\n  %d nodes\n", 
            walks, timeTotal, int ( float(walks) / timeTotal), i);

}

//--------------------------------------------------------------------- 

void Benchmark::benchmarkSearch() const
{
  Engine* engine = new Engine();
  Board *playBoard = new Board(*board_);

  engine->doSearch(playBoard);
  logRaw(engine->getStatistics().c_str());
}



//--------------------------------------------------------------------- 

void Benchmark::benchmarkAll() 
{
  benchmarkEval();
  benchmarkPlayout();
  benchmarkUct();
  benchmarkSearch();
}

//--------------------------------------------------------------------- 
//--------------------------------------------------------------------- 


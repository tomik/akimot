"""
   Arimaa rabbit test. 
"""
import socket
import sys
import logging

#for importing aei
sys.path.append('..')

logging.basicConfig(
        level = logging.DEBUG,
        datefmt="%Y-%m-%d %H:%M:%S",
        format="%(asctime)s %(levelname)s:%(name)s:%(message)s",
        stream = sys.stdout,
        )

log = logging.getLogger("ats")

from ConfigParser import SafeConfigParser

from aei import board
from aei.aei import StdioEngine, EngineController 


def do_test(engine, pos_str, move):
    pos = board.parse_short_pos(board.COL_SILVER, 4, pos_str)

    engine.newgame()
    engine.setposition(pos)
    engine.engine.send('goalcheck\n')

    res = ''
    while True:
        resp = engine.get_response()
        if resp.type == "info":
            lmsg = resp.message.split(' ')
            if lmsg[0] == 'goalcheck':
              result  = lmsg[1]
              extra = ' '.join(lmsg[2:])
              break


    wrong = {}
    if result not in  ['gold', 'silver']:
        wrong['result'] = True
    else:
        goal_pos = pos.do_move(board.parse_move(extra))

        if not goal_pos.is_goal():
            wrong['goal'] = True

    
    if (len(wrong) > 0):
        print "========================="
        print pos.to_long_str()
        print result

        if wrong.has_key('goal'):
            print "Wrong goal move generated"
            print extra 
        if wrong.has_key('result'):
            print "should be", move

if __name__ == '__main__':
    try:
        rabbits_pos_fn = sys.argv[1]
    except IndexError:
        print "Usage: %s rabbits_pos_file" % sys.argv[0]
        sys.exit(1)

    engine_cmd = './akimot -c default.cfg -l'
    engine = EngineController(StdioEngine(engine_cmd, log))

    for pos, move in map(lambda x: x.split('#'), open(rabbits_pos_fn, 'r').readlines()):
        do_test(engine, pos.strip(), move.strip())

    engine.quit()
    engine.cleanup()

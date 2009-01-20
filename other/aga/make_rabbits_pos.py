import sys

sys.path.append('..')
from aei import board

def make_empty_pos():
    bitboards = list([list(x) for x in board.BLANK_BOARD])
    return board.Position(board.COL_GOLD, 4, bitboards)

def process_record(line):
    moves = line.split('\\n')
    pos = make_empty_pos()
    for m in moves: 
        #print m
        #strip off move number
        m = m[m.find(' ') + 1:]
        try: 
            steps = board.parse_move(m)
            lastpos = pos
            pos = pos.do_move(steps)
        except ValueError:
            break
    if pos.is_goal():
        print lastpos.to_short_str()
    
if __name__ == '__main__':
    
    if len(sys.argv) != 2:
        print "usage:%s records_file", sys.argv[0]
        sys.exit(1)

    fp = open(sys.argv[1], 'r')
    
    for line in fp.readlines():
        process_record(line)         


import sys

#cols = ['id', 'wplayerid', 'bplayerid', 'wusername', 'busername', 'wtitle', 'btitle', 'wcountry', 'bcountry', 'wrating', 'brating', 'wtype', 'btype', 'event', 'site', 'timecontrol', 'postal', 'startts', 'endts', 'result', 'termination', 'plycount', 'mode', 'rated', 'movelist', 'events']

MIN_RATING = 1300

FILTERS = [
           #('rated', lambda x: bool(x) == True), 
           ('termination', lambda x: x == 'g'), 
           (('wrating', 'brating'), lambda x, y: int(x) > MIN_RATING and int(y) > MIN_RATING)
          ]

SPLIT_CHAR = '\t'

#PROCESSORS = ['id', 'result', 'movelist']
PROCESSORS = ['movelist']


def process_game(cols, game_line):
    tokens = game_line.split(SPLIT_CHAR)
    return SPLIT_CHAR.join(map(lambda x: tokens[cols[x]], PROCESSORS))

def game_ok(filters, cols, game_line):
    tokens = game_line.split(SPLIT_CHAR)
    for fields, func in filters: 
        if not isinstance(fields, tuple):
            fields = (fields, )
        fields = map(lambda x: tokens[cols[x]], fields)
        if not func(*(fields)):
            return False
    return True
        
def parse_games_file(fn):
    lines = open(fn, 'r').readlines()
    #columns dictionary name: order
    cols = dict(map(lambda x: (x[1], x[0]), enumerate(lines[0].strip().split(SPLIT_CHAR))))
    res = []
    
    for line in filter(lambda x: game_ok(FILTERS, cols, x), lines[1:]):
        res.append(process_game(cols, line))
    for r in res:
        print r 
    #print "parsed %d from %s" % (len(res), fn)

def parse_games_file_list(list_fn):
    #print "parsing ", list_fn
    for fn in map( lambda x: x.strip(), open(list_fn, 'r').readlines()):
        parse_games_file(fn)

if __name__ == '__main__': 
    
    if len(sys.argv) != 2:
        print "usage:%s games_file_list", sys.argv[0]
        sys.exit(1)

    parse_games_file_list(sys.argv[1])


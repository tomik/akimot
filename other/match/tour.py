import os
import sys
import time
import shutil

MATCH_DIR = "matches"
RECORD_FILE = "record"
DIR_LIST = "list.txt"
LOG_FILE = "log"

MODES=['standalone', 'finish_only', 'master', 'slave', 'test']

def setup(opt):
    try:
        os.mkdir(opt.game_dir)
    except: 
        pass

def default_game_dir():
    try:
        os.mkdir(MATCH_DIR)
    except: 
        pass
    
    #determine dir name
    files = map(int, filter(lambda x: x.isdigit(), os.listdir(MATCH_DIR)))
    files.append(0)
    inner_dir = max(files) + 1

    game_dir = "%s/%s" % (MATCH_DIR, inner_dir)
    return game_dir


def get_result(record):
    lines = open(record,"r").readlines()
    if lines[-1].startswith('w '): 
        return 'w'
    elif lines[-1].startswith('b' ): 
        return 'b' 
    return ' '


def run(bots, opt):
    results = []
    for i in xrange(opt.start_from, opt.start_from + opt.games_num):
        print "running %d/%d score so far: %d x %d" % (i - opt.start_from + 1, opt.games_num, results.count('w'), results.count('b'))
        record = "%s/%s_%d" % (opt.game_dir, RECORD_FILE, i)
        log = "%s/%s_%d" % (opt.game_dir, LOG_FILE, i) 
        cmdline = "./match %s %s 1>%s 2>%s" % (bots[0], bots[1], record, log)

        if os.system(cmdline) == 2:
            print "\nInterrupted ..."
            break
        results.append(get_result(record))

def finish(opt):
    #copy configuration files
    for fn in [bots[0], bots[1]]:
      fn = "bot_akimot/%s.cfg" % fn 
      try:
        shutil.copy(fn, opt.game_dir)
      except: 
        pass

    #save setup message

    f = open("%s/%s" % (opt.game_dir,"setup"),"w")
    f.write(opt.setup_msg)
    f.close()

    records = filter(lambda x: x.startswith(RECORD_FILE), os.listdir(opt.game_dir))
    results = [get_result("%s/%s" % (opt.game_dir, r)) for r in records]  

    list = open("%s/../%s" % (opt.game_dir, DIR_LIST),"a")
    list.write("%s --- played at %s : %s\n" % (opt.game_dir, opt.timestamp, opt.setup_msg.replace("\n"," ")))
    list.write("        result: %d - %d\n" % (results.count('w'), results.count('b')))
    list.close()

if __name__ == "__main__":

    from optparse import OptionParser

    parser = OptionParser()
    parser.add_option("--games_num", "-n", help="number of matches", default=1, type="int")
    parser.add_option("--comment", "-c", help="comment :)", default="")
    parser.add_option("--game_dir", "-d", help="use given dir to store game records", default=default_game_dir())
    parser.add_option("--start_from", "-s", help="start numbering games from ...", default=1, type="int")
    parser.add_option("--mode", "-m", help="What mode running in ... options are [standalone(implicit), master, slave]", default=MODES[0])

    (options, args) = parser.parse_args()
    
    if len(args) != 2:
        print "usage: bot1 bot2 ..." 
        sys.exit(1)
    if options.mode not in MODES:
        print "Illegal value for mode .. see help"
        sys.exit(1)

    bots = (args[0], args[1])

    options.timestamp = time.strftime("%Y-%m-%d-%H-%M-%S")
    options.setup_msg = "%s x %s \n%s matches\n%s" % (bots[0], bots[1], options.games_num, options.comment)

    if options.mode == 'standalone': 
        setup(options)
        run(bots, options)
        finish(options)
    elif options.mode == 'slave': 
        setup(options)
        run(bots, options)
    elif options.mode == 'finish_only': 
        finish(options)
    elif options.mode == 'master': 
        from psshlib import work, MS_CREW
        JOB = 'cd $MATCH;python %s %s %s --mode slave --game_dir %s --start_from %%d' % (sys.argv[0], bots[0], bots[1], options.game_dir) 
        jobs = [JOB % i for i in xrange(options.start_from, options.start_from + options.games_num)] 
        work(MS_CREW, 25, jobs);
        job = 'cd $MATCH;python %s %s %s --mode finish_only --game_dir %s --games_num %d --comment "%s"' \
                % (sys.argv[0], bots[0], bots[1], options.game_dir, options.games_num, options.comment)
        work(MS_CREW, 1, [job]);
    elif options.mode == 'test': 
        from psshlib import work, MS_CREW
        print "MS WORK TEST"

        jobs = []
        for i in xrange(10):
            jobs.append('echo "%d"' % i)
        work(MS_CREW, 3, jobs);
        

import os
import sys
import time
import shutil

match_dir = "matches"
record_file = "record"
dir_list = "list.txt"
log_file = "log"

def runTournament(bots, matches, comment):

    bot_1 = bots[0]
    bot_2 = bots[1]

    try:
        os.mkdir(match_dir)
    except: 
        pass

    #determine file name
    files = map(int, filter(lambda x: x.isdigit(), os.listdir(match_dir)))
    files.append(0)
    top = max(files) + 1

    dir = "%s/%s" % (match_dir, top)
    os.mkdir(dir)

    timestamp = time.strftime("%Y-%m-%d-%H-%M-%S")
    setup_msg = "%s x %s \n%s matches\n%s" % (bot_1, bot_2, matches, comment)

    list = open("%s/%s" % (match_dir, dir_list),"a")
    list.write("%s --- played at %s : %s\n" % (top, timestamp, setup_msg.replace("\n"," ")))

    for fn in [bot_1, bot_2]:
      fn = "bot_akimot/%s.cfg" % fn 
      try:
        shutil.copy(fn, dir)
      except: 
        pass

    f = open("%s/%s" % (dir,"setup"),"w")
    f.write(setup_msg)
    f.close()

    bot_1_win = 0
    bot_2_win = 0

    for i in xrange(matches):
        print "Match %d/%d" % (i+1, matches)
        record = "%s/%s_%d" % (dir, record_file, i + 1)
        log = "%s/%s_%d" % (dir, log_file, i + 1)
        cmdline = "./match %s %s 1>%s 2>%s" % (bot_1, bot_2, record, log)
        if os.system(cmdline) == 2:
            print "\nInterrupted ..."
            break
        lines = open(record,"r").readlines()
        if lines[-1][0] == 'w': 
            bot_1_win += 1 
        else:
          if lines[-1][0] == 'b': 
            bot_2_win += 1 

    print "%d x %d" % (bot_1_win, bot_2_win)
    list.write("        result: %d - %d\n" % (bot_1_win, bot_2_win))
    list.close()


if __name__ == "__main__":

    from optparse import OptionParser

    parser = OptionParser()
    parser.add_option("--matches", "-m", help="number of matches", metavar="matches", default=100, type="int")
    parser.add_option("--comment", "-c", help="comment :)", metavar="comment" )

    (options, args) = parser.parse_args()
    
    if len(args) != 2:
        print "usage: bot1 bot2 ..." 
        sys.exit(1)

    bot_1 = args[0]
    bot_2 = args[1]

    if options.comment is None:
        options.comment = ""
    
    runTournament([bot_1, bot_2], options.matches, options.comment)
    

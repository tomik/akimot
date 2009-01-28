import os
import sys
import time

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

    timestamp = time.strftime("%m%d%H%M%S")
    dir = "%s/%s" % (match_dir, timestamp)
    os.mkdir(dir)

    setup_msg = "%s x %s \n%s matches\n%s" % (bot_1, bot_2, matches, comment)

    list = open("%s/%s" % (match_dir, dir_list),"a")
    list.write("%s --- %s\n" % (timestamp, setup_msg.replace("\n"," ")))
    list.close()

    f = open("%s/%s" % (dir,"setup"),"w")
    f.write(setup_msg)
    f.close()

    bot_1_win = 0

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

    print "%d/%d" % (bot_1_win, matches)


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
        options.comment = "%s vs. %s" % (bot_1, bot_2)
    
    runTournament([bot_1, bot_2], options.matches, options.comment)
    

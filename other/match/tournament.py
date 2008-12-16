import os
import time

bot_1 = "akimot"
bot_2 = "sample1"
matches = 100
match_dir = "matches"
record_file = "record"
log_file = "log"

try:
    os.mkdir(match_dir)
except: 
    pass

timestamp = time.strftime("%m%d%H%M%S")
dir = "%s/%s" % (match_dir, timestamp)
os.mkdir(dir)

bot_1_win = 0

for i in xrange(matches):
    print "Match %d/%d" % (i+1, matches)
    record = "%s/%s_%d" % (dir, record_file, i+1)
    log = "%s/%s_%d" % (dir, log_file, i)
    cmdline = "./match %s %s 1>%s 2>%s" % (bot_1, bot_2, record, log)
    if os.system(cmdline) == 2:
        print "\nInterrupted ..."
        break
    lines = open(record,"r").readlines()
    if lines[-1][0] == 'w': 
        bot_1_win += 1 

print "%d/%d" % (bot_1_win, matches)

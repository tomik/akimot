"""
  Arimaa Test Suite
"""
import logging
import socket
import sys
import re

#for importing aei
sys.path.append('..')

logging.basicConfig(
        level = logging.DEBUG,
        datefmt="%Y-%m-%d %H:%M:%S",
        format="%(asctime)s %(levelname)s:%(name)s:%(message)s",
        stream = sys.stdout,
        #filename = 'log',
        )

log = logging.getLogger("ats")

from aei import board
from aei.aei import StdioEngine, EngineController 

import tests

CYCLES = 100
TIME_PER_TEST = 0.2

class Test(object):
    """
        One test.
    """
    def __init__(self, test_file):
        self.mod_name = test_file.split('.py')[0]
        self.mod = __import__(self.mod_name) 

    def do_test(self, engine):
        lines = self.mod.pos.strip('\n').split('\n')
        movenum, pos = board.parse_long_pos(lines)

        log.debug('\n' + pos.to_long_str())

        engine.newgame()
        engine.setoption("tcmove", TIME_PER_TEST)
        engine.setposition(pos)
        engine.go()

        log.debug("Doing the test %s." % self) 
        while True:
            resp = engine.get_response()
            if resp.type == "info":
                log.debug(resp.message)
                lmsg = resp.message.split(' ')
                if lmsg[0] == 'winratio':
                    winratio = float(lmsg[1])
                    break
            elif resp.type == "log":
                log.debug( "log: %s" % resp.message)
            elif resp.type == "bestmove":
                log.debug( "bestmove: %s" % resp.move)


        if winratio >= self.mod.win_ratio:
            return True
        return False

    def __str__(self):
        return "%s" % (self.mod_name)
        #return "Test %s: %s." % (self.mod_name ,self.mod.__doc__)


class Suite:
    """
        Some collection of tests.
    """
    def __init__(self, test_files, engine_cmd):
        self.tests = [ Test(test_file) for test_file in test_files]
        self.engine = EngineController(StdioEngine(engine_cmd, log))

    def run(self):
        print("Running the test suite...") 
        passed = {}
        i = 0
        for test in self.tests:
            for c in xrange(CYCLES):
                i += 1 
                log.debug("trial %d", i) 
                if test.do_test(self.engine):
                    passed[test] = passed.get(test, 0) + 1 
                    log.debug("%s passed.", test) 
                else:
                    log.debug("%s failed.", test) 
                
        passed_num = sum(passed.values())
        for test in self.tests:
            print ("%s passed %d/%d times." % (test, passed.get(test,0), CYCLES))
        print("Passed %d/%d tests." % (passed_num, CYCLES * len(self.tests)))
    
    def __del__(self):
        self.engine.quit()
        self.engine.cleanup()

def filter_test_files(filter, test_files):
    if filter == 'all' or filter == '*':
        return test_files
    l = []
    for item in filter.split(' '):
        left, right = (int(item.split('-')[0]), int(item.split('-')[-1]))
        l += range(left, right + 1)
    d = dict(zip(l,l))
    test_files_nums = [ int(re.sub(r'[a-z/]*([0-9])*.*',r'\1',f)) for f in test_files]
    return [ t for t,n in zip(test_files, test_files_nums) if d.has_key(n)]

if __name__ == '__main__':
    from ConfigParser import SafeConfigParser

    try:
        config_file = sys.argv[1]
    except IndexError:
        log.error("Usage: %s config_file." % sys.argv[0])
        sys.exit(1)
      
    config = SafeConfigParser()
    try:
        config.readfp(open(config_file, 'rU'))
    except IOError:
        log.error("Could not open configuration file: %s." % config_file)
        sys.exit(1)
    
    engine = config.get("global", "engine")
    filter = config.get("global","tests","*")
    s = Suite(filter_test_files(filter, tests.get_test_files()), engine) 
    s.run()


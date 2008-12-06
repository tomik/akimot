"""
  Arimaa Test Suite
"""
import logging
import socket
import sys

#for importing aei
sys.path.append('..')

logging.basicConfig(
        level = logging.DEBUG,
        datefmt="%Y-%m-%d %H:%M:%S",
        format="%(asctime)s %(levelname)s:%(name)s:%(message)s",
        #stream = sys.stdout,
         filename = 'log',
        )

log = logging.getLogger("ats")

from aei import board
from aei.aei import StdioEngine, EngineController 

import tests

CYCLES = 500
TIME_PER_TEST = 0.2

class Test(object):
    """
        One test.
    """
    def __init__(self, test_file):
        self.mod_name = test_file.split('.py')[0]
        self.mod = __import__(self.mod_name) 

    def do_test(self, engine):
        log.debug("Doing the test %s." % self) 
        lines = self.mod.pos.strip('\n').split('\n')
        movenum, pos = board.parse_long_pos(lines)

        log.debug('\n' + pos.to_long_str())

        engine.setoption("tcmove", TIME_PER_TEST)
        engine.newgame()
        engine.setposition(pos)
        engine.go()

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
        #unit_total = CYCLES * len(self.tests)
        #unit = float(unit_total) / 100
        #unit_finished = 0
        i = 0
        for c in xrange(CYCLES):
            for test in self.tests:
                i += 1 
                log.debug("trial %d", i) 
                if test.do_test(self.engine):
                    passed[test] = passed.get(test, 0) + 1 
                    log.debug("%s passed.", test) 
                else:
                    log.debug("%s failed.", test) 
                #for p in xrange(i/unit - unit_finished):
                #    unit_finished += 1
                #    print r"-" 
                
        passed_num = sum(passed.values())
        for test in self.tests:
            print ("%s passed %d/%d times." % (test, passed.get(test,0), CYCLES))
        print("Passed %d/%d tests." % (passed_num, CYCLES * len(self.tests)))
    
    def __del__(self):
        self.engine.quit()
        self.engine.cleanup()


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
    s = Suite(tests.get_test_files(), engine) 
    s.run()


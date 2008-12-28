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

from ConfigParser import SafeConfigParser

from aei import board
from aei.aei import StdioEngine, EngineController 

import tests

DEFAULT_CYCLES = 100
DEFAULT_TIME_PER_TEST = 5

class ConfigWrapper(object):

    def __init__(self, config):
        self.config = config

    def __getitem__(self, key):
        return self.config.get(DEFAULT_SECTION, key)

    def __getattr__(self, attr, *args, **kwargs):
        for section in self.config.sections():
            try:
                return self.config.get(section, attr)
            except :
                pass
        raise AttributeError

class TimeSettings(object):
    """
        Holds the time settings for tests.
    """
    def __init__(self, cycles=DEFAULT_CYCLES, time_per_test=DEFAULT_TIME_PER_TEST):
        self.cycles = int(cycles)
        self.time_per_test = float(time_per_test)

class Test(object):
    """
        One test.
    """
    def __init__(self, test_file, time_per_test):
        self.time_per_test = time_per_test
        test_config = SafeConfigParser()
        try:
            test_config.readfp(open(test_file,"rU"))
        except IOError:
            log.error("Wrong test file format in %s." % test_file)
            sys.exit(1)
        self.test = ConfigWrapper(test_config)
        self.test.name = test_file
        print "Creating test %s" % self.test.name

    def do_test(self, engine):
        lines = self.test.position.strip('\n').split('\n')
        movenum, pos = board.parse_long_pos(lines)

        engine.newgame()
        engine.setoption("tcmove", self.time_per_test)
        engine.setposition(pos)
        engine.go()

        log.debug('\n' + pos.to_long_str())
        log.debug("Doing the test %s." % self) 
        while True:
            resp = engine.get_response()
            if resp.type == "info":
                log.debug(resp.message)
                lmsg = resp.message.split(' ')
                if lmsg[0] == 'winratio':
                  winratio = float(lmsg[1])
                  #break
            elif resp.type == "log":
                log.debug( "log: %s" % resp.message)
            elif resp.type == "bestmove":
                log.debug( "bestmove: %s" % resp.move)
                bestmove = resp.move
                break

        if self.test.condition == "score goal":
            if pos.do_move(board.parse_move(bestmove)).is_goal():
                return True
            return False

        if self.test.condition == "prevent goal":
            new_pos = pos.do_move(board.parse_move(bestmove))
            if pos.is_goal():
                return True
            for p, move in new_pos.get_moves().items():
                if p.is_goal():
                    print "Opponent's goal by:", board.steps_to_str(move)
                    return False
            return True 

        raise Exception("Unknown condition.")

        #judge by winratio OBSOLETE 
        #if self.test.condition == "winratio":
        #    if winratio >= float(self.test.win_ratio):
        #        return True
        #    return False
            

    def __str__(self):
        return "%s" % (self.test.name)

class Suite:
    """
        Some collection of tests.
    """
    def __init__(self, test_files, engine_cmd, time_settings):
        self.engine = EngineController(StdioEngine(engine_cmd, log))
        self.time_settings = time_settings
        self.tests = [ Test(test_file, self.time_settings.time_per_test) for test_file in test_files]

    def run(self):
        print("Running the test suite...") 
        passed = {}
        i = 0
        for test in self.tests:
            for c in xrange(self.time_settings.cycles):
                i += 1 
                log.debug("trial %d", i) 
                if test.do_test(self.engine):
                    passed[test] = passed.get(test, 0) + 1 
                    log.debug("%s passed.", test) 
                else:
                    log.debug("%s failed.", test) 
                
        passed_num = sum(passed.values())
        for test in self.tests:
            print ("%s passed %d/%d times." % (test, passed.get(test,0), self.time_settings.cycles))
        print("Passed %d/%d tests." % (passed_num, self.time_settings.cycles * len(self.tests)))
    
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
    time_settings = TimeSettings(cycles=config.get("global", "cycles", DEFAULT_CYCLES), time_per_test=config.get("global", "time_per_test", DEFAULT_TIME_PER_TEST) )
    s = Suite(filter_test_files(filter, tests.get_test_files()), engine, time_settings) 
    s.run()


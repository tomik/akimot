"""
  Arimaa Test Suite
"""
import logging
import socket
import sys

#for importing aei
sys.path.append('..')

from aei import board
from aei.aei import StdioEngine, EngineController 

import tests


logging.basicConfig(level = logging.DEBUG,
        #filename = logfilename,
        stream = sys.stderr, 
        datefmt="%Y-%m-%d %H:%M:%S",
        format="%(asctime)s %(levelname)s:%(name)s:%(message)s",
        )

class Test(object):
    """
        One test.
    """
    def __init__(self, test_file):
        self.mod_name = test_file.split('.py')[0]
        self.mod = __import__(self.mod_name) 

    def do_test(self, engine):
        logging.info("Doing the test %s." % self) 
        lines = self.mod.pos.strip('\n').split('\n')
        movenum, pos = board.parse_long_pos(lines)

        print pos.to_long_str()

        engine.setoption("tcmove", 2)
        engine.setposition(pos)
        engine.go()

        while True:
            resp = engine.get_response()
            if resp.type == "info":
                print resp.message
            elif resp.type == "log":
                print "log: %s" % resp.message
            elif resp.type == "bestmove":
                print "bestmove: %s" % resp.move
                break

        engine.quit()
        engine.cleanup()

    def __str__(self):
        return "Test %s: %s." % (self.mod_name ,self.mod.__doc__)


class Suite:
    """
        Some collection of tests.
    """
    def __init__(self, test_files, engine_cmd):
        self.tests = [ Test(test_file) for test_file in test_files]
        self.engine = EngineController(StdioEngine(engine_cmd, logging))

    def run(self):
       logging.info("Running the test suite.") 
       for test in self.tests:
           test.do_test(self.engine)


if __name__ == '__main__':
    from ConfigParser import SafeConfigParser

    try:
        config_file = sys.argv[1]
    except IndexError:
        print "Usage: %s config_file." % sys.argv[0]
        sys.exit(1)
      
    config = SafeConfigParser()
    try:
        config.readfp(open(config_file, 'rU'))
    except IOError:
        print "Could not open configuration file: %s." % config_file
        sys.exit(1)
    
    engine = config.get("global", "engine")
    s = Suite(tests.get_test_files(), engine) 
    s.run()


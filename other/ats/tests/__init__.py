""""""
"""
"""
import os 

TEST_PREFIX = 'test'

def get_test_files():
    mod = __import__(__name__)
    dir = mod.__file__.rpartition('/')[0] 
    files = [ "%s/%s" % (dir.split('/')[-1], file) for file in os.listdir(dir) 
              if file.startswith(TEST_PREFIX) and file.endswith('.py')]
    return files


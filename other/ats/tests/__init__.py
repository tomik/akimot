"""
    Defines function for getting all test files from this directory.
"""
import os 

def get_test_files():
    dir = __file__.rpartition('/')[0] 
    files = [ "%s/%s" % (dir.split('/')[-1], file) for file in os.listdir(dir) 
              if file.endswith('.tst')]
    return files



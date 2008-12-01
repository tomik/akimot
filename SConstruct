#!/usr/bin/python 

import paths
CXX_TEST_PATH = paths.CXX_TEST_PATH
CXX_INCLUDE_DIR = paths.CXX_INCLUDE_DIR

AKIMOT_AEI_DIR = paths.AKIMOT_AEI_DIR 
AKIMOT_MATCH_DIR = paths.AKIMOT_MATCH_DIR 
AKIMOT_LIBS = ['pthread']

common = Environment(CC='g++', CCFLAGS = '-Wall ')
opt = common.Clone()
opt.Append(CCFLAGS = '-O3 -DNDEBUG -ffast-math -fomit-frame-pointer -frename-registers') #-march=native
dbg = common.Clone()
dbg.Append(CCFLAGS = '-ansi -DDEBUG_1 -DDEBUG_2 -DDEBUG_3 ')
std = common.Clone()
std.Append(CCFLAGS = '')

src_files_common = 'board.cpp engine.cpp utils.cpp benchmark.cpp eval.cpp config.cpp hash.cpp aei.cpp timer.cpp'.split()
src_files_build = src_files_common + ['main.cpp'] 
src_files_test = src_files_common 
#todo - is this portable ? determine the extension of object file ( '.o' at linux) dynamically
obj_files_build = [src_file[:src_file.rindex('.')] + '.o' for src_file in src_files_build]
obj_files_test = [src_file[:src_file.rindex('.')] + '.o' for src_file in src_files_test]

do_build = False

if ARGUMENTS.get('opt'): 
  env = opt.Clone()
  do_build = True
elif ARGUMENTS.get('dbg'):
  env = dbg.Clone()
  do_build = True
elif ARGUMENTS.get('doc'):
  bld = Builder(action = '/usr/bin/doxygen Doxy')
  doc = Environment(BUILDERS = {'Foo' : bld})
  doxy = doc.Foo(source=src_files_build)
else:   #standard
  env = std.Clone()
  do_build = True

if do_build:
  #akimot = env.Program(target = 'akimot', source = src_files_build, CPPPATH = '.')
  env.Object(src_files_build)
  akimot = env.Program(target = 'akimot', source = obj_files_build, LIBS = AKIMOT_LIBS, CPPPATH = '.')
  env.Install(AKIMOT_AEI_DIR, akimot)
  env.Alias('aei', AKIMOT_AEI_DIR)
  env.Install(AKIMOT_MATCH_DIR, akimot)
  env.Alias('match', AKIMOT_MATCH_DIR)
  tst = Environment(tools = ['default','cxxtest'], CXXTEST=CXX_TEST_PATH, LIBS = AKIMOT_LIBS, CXXTEST_DIR='', 
                    CPPPATH=['.',CXX_INCLUDE_DIR])
  tst.CxxTest('func_test', ['tests.h'] + obj_files_test)
  tst.CxxTest('perf_test', ['performance.h'] + obj_files_test)
  #print "CCCOM is ",  env.subst('$CCCOM')

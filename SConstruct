#!/usr/bin/python 

import paths
CXX_TEST_PATH = paths.CXX_TEST_PATH
CXX_INCLUDE_DIR = paths.CXX_INCLUDE_DIR
AKIMOT_AEI_DIR = paths.AKIMOT_AEI_DIR 
AKIMOT_ATS_DIR = paths.AKIMOT_ATS_DIR
AKIMOT_MATCH_DIR = paths.AKIMOT_MATCH_DIR 
RABBITS_TEST_DIR = paths.RABBITS_TEST_DIR
alias_dirs = [('aei',AKIMOT_AEI_DIR), ('ats', AKIMOT_ATS_DIR), ('match', AKIMOT_MATCH_DIR), ('rt', RABBITS_TEST_DIR)] 

AKIMOT_LIBS = ['pthread']

TARGET = 'akimot'
OPT_TARGET = TARGET #'opt_akimot'

common = Environment(CC='g++', CCFLAGS = '-Wall ', LINKFLAGS = '')
opt = common.Clone()
opt.Append(CCFLAGS = '-O3 -DNDEBUG -ffast-math -fomit-frame-pointer -frename-registers') #-march=native
dbg = common.Clone()
dbg.Append(CCFLAGS = '-ansi -DDEBUG_1 -DDEBUG_2 -DDEBUG_3 ')
prof = common.Clone()
prof.Append(CCFLAGS = '-O1 -DNEDBUG -ffast-math -g -pg', LINKFLAGS = '-pg') 
std = common.Clone()
std.Append(CCFLAGS = '')

src_files_common = 'board.cpp engine.cpp uct.cpp utils.cpp benchmark.cpp eval.cpp config.cpp hash.cpp aei.cpp timer.cpp'.split()
src_files_build = src_files_common + ['main.cpp'] 
src_files_test = src_files_common 
#todo - is this portable ? determine the extension of object file ( '.o' at linux) dynamically
obj_files_build = [src_file[:src_file.rindex('.')] + '.o' for src_file in src_files_build]
obj_files_test = [src_file[:src_file.rindex('.')] + '.o' for src_file in src_files_test]

do_build = False
do_prof = False

if ARGUMENTS.get('opt'): 
    env = opt.Clone()
    do_build = True
    TARGET = OPT_TARGET
elif ARGUMENTS.get('dbg'):
    env = dbg.Clone()
    do_build = True
elif ARGUMENTS.get('prof'):
    env = prof.Clone()
    do_build = True
elif ARGUMENTS.get('doprof'):
    do_prof = True

elif ARGUMENTS.get('doc'):
    bld = Builder(action = '/usr/bin/doxygen Doxy')
    doc = Environment(BUILDERS = {'Foo' : bld})
    doxy = doc.Foo(source=src_files_build)
elif ARGUMENTS.get('cfg'): 
    import shutil
    for alias, dir in alias_dirs: 
        shutil.copy("default.cfg", dir)
        print "copying default.cfg to ", dir
else:   #standard
  env = std.Clone()
  do_build = True


if do_build:
    env.Object(src_files_build)
    akimot = env.Program(target = TARGET, source = obj_files_build, LIBS = AKIMOT_LIBS, CPPPATH = '.')
    for alias, dir in alias_dirs: 
        env.Alias(alias, dir)
        env.Install(dir, akimot)
    tst = Environment(tools = ['default','cxxtest'], CXXTEST=CXX_TEST_PATH, LIBS = AKIMOT_LIBS, CXXTEST_DIR='', 
                   CPPPATH=['.',CXX_INCLUDE_DIR])
    tst.CxxTest('do_tests', ['tests.h'] + obj_files_test)
    #print "CCCOM is ",  env.subst('$CCCOM')

if do_prof:
    import os
    os.system('./akimot -b; gprof akimot gmon.out | head -n 45');


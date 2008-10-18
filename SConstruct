#!/usr/bin/python 

common = Environment(CC='g++', CCFLAGS = '-Wall ')
opt = common.Clone()
opt.Append(CCFLAGS = '-O3 -DNDEBUG -march=native -ffast-math -fomit-frame-pointer -frename-registers')
dbg = common.Clone()
dbg.Append(CCFLAGS = '-ansi -DDEBUG_1 -DDEBUG_2 -DDEBUG_3 ')
std = common.Clone()
std.Append(CCFLAGS = '')


src_files = Split('board.cpp engine.cpp utils.cpp getMove.cpp benchmark.cpp eval.cpp config.cpp')

if ARGUMENTS.get('opt'):
  env = opt.Clone()
elif ARGUMENTS.get('dbg'):
  env = dbg.Clone()
elif ARGUMENTS.get('doc'):
  #doc = Environment(tools = ["default", "doxygen"], toolpath = '/usr/lib/python2.5/site-packages/' )
  #doc.Doxygen("doxygen.conf")
  bld = Builder(action = '/usr/bin/doxygen Doxy')
  doc = Environment(BUILDERS = {'Foo' : bld})
  doxy = doc.Foo(source=src_files)
    
else:   #standard
  env = std.Clone()

if not ARGUMENTS.get('doc'):      #dirty - change !
  env.Program(target = 'akimot', source = src_files, CPPPATH = '.')
  print "CCCOM is ",  env.subst('$CCCOM')


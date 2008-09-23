#!/usr/bin/python 

common = Environment(CC='g++ -Wall')
opt = common.Clone(CCFLAGS = '-O3 -DNDEBUG -march=native -ffast-math -fomit-frame-pointer -frename-registers')
dbg = common.Clone(CCFLAGS = '-ansi -DDEBUG_1 -DDEBUG_2 -DDEBUG_3 ')
std = common.Clone(CCFLAGS = '')

src_files = Split('board.cpp engine.cpp utils.cpp getMove.cpp')

if ARGUMENTS.get('opt'):
  env = opt.Clone()
elif ARGUMENTS.get('dbg'):
  env = dbg.Clone()
else:   #standard
  env = std.Clone()

env.Program(target = 'akimot', source = src_files,CPPPATH='.')
print "CCCOM is ",  env.subst('$CCCOM')


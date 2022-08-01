# RT-Thread building script for bridge

from building import *

cwd      = GetCurrentDir()
src      = Glob('src/*.c')
src     += Glob('util/*.c')

CPPPATH  = [cwd + '/inc']
CPPPATH += [cwd + '/util']

group = DefineGroup('agile_modbus', src, depend = ['PKG_USING_AGILE_MODBUS'], CPPPATH = CPPPATH)

Return('group')

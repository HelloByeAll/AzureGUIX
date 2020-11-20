
from building import *
import rtconfig

cwd     = GetCurrentDir()
src     = Glob('Source/common/src/*.c')
CPPPATH = [cwd + 'Source/common/inc']


src += Glob('Port/gx_system_rtos_bind_rtthread.c')

group = DefineGroup('GUIX/Source', src, depend = ['PKG_USING_AZUREGUIX'], CPPPATH = CPPPATH)

cwd     = GetCurrentDir()
src     = Glob('Port/gx_user.h')
CPPPATH = [cwd + 'Port']
group = DefineGroup('GUIX/Port', src, depend = ['PKG_USING_AZUREGUIX'], CPPPATH = CPPPATH)

cwd     = GetCurrentDir()
src     = Glob('Examples/*.c')
CPPPATH = [cwd + 'Examples']
group = DefineGroup('GUIX/Examples', src, depend = ['PKG_USING_AZUREGUIX'], CPPPATH = CPPPATH)

Return('group')

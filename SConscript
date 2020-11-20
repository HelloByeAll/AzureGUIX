
from building import *
import rtconfig

cwd     = GetCurrentDir()
src     = Glob('Source/common/src/*.c')
CPPPATH = [cwd + 'Source/common/inc']
LOCAL_CCFLAGS = ''

if GetDepend(['PKG_USING_AZUREGUIX']):
    src += Glob('Port/gx_system_rtos_bind_rtthread.c')

group = DefineGroup('GUIX/Source', src, depend = ['PKG_USING_AZUREGUIX'], CPPPATH = CPPPATH, LOCAL_CCFLAGS = LOCAL_CCFLAGS)

Return('group')

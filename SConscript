
from building import *
import rtconfig

cwd     = GetCurrentDir()
src     = Glob('Source/common/src/*.c')
CPPPATH = [cwd + 'Source/common/inc']
LOCAL_CCFLAGS = ''

if GetDepend(['GUIX_BINDING_RT_THREAD']):
    src += Glob('Port/gx_system_rtos_bind_rtthread.c')

group = DefineGroup('GUIX/Source', src, depend = ['GX_THREADX_BINDING'], CPPPATH = CPPPATH, LOCAL_CCFLAGS = LOCAL_CCFLAGS)

Return('group')

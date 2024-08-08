# -*- python -*-
from SCons.Script import COMMAND_LINE_TARGETS
import sys

if sys.platform == "darwin":
    def ipmbase(env):
        env.Append(CXXFLAGS=['-std=c++14'])
    env = Environment(tools=['default', ipmbase])
else:
    env = Environment(tools=['default'])


Export('env')


sources = Split("""
ctrl.cc
naiipm.cc
src/measure.cc
src/status.cc
""")


ipm_ctrl=env.Program(target = 'ipm_ctrl', source = sources)
env.Default(ipm_ctrl)

env.Alias('install', env.Install('/opt/nidas/bin', 'ipm_ctrl'))

env.SConscript("tests/SConscript")

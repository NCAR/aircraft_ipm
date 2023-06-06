env = Environment()
sources = Split("""
ctrl.cc
naiipm.cc
""")

ipm_ctrl=env.Program(target = 'ipm_ctrl', source = sources)
env.Default(ipm_ctrl)

env.Alias('install', env.Install('/opt/nidas/bin', 'ipm_ctrl'))

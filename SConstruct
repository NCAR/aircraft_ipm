env = Environment()
sources = Split("""
ctrl.cc
naiipm.h
naiipm.cc
""")

ipm_ctrl=env.Program(target = 'ipm_ctrl', source = sources)
env.Default(ipm_ctrl)

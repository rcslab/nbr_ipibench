
env = Environment()

# Use system clang
env["CC"] = "cc"
env["LD"] = "cc"
env["CFLAGS"] = "-pthread"
env["LINKFLAGS"] = "-pthread"

env.Program("ipitest", "ipitest.c")
env.Program("sigtest", "sigtest.c")


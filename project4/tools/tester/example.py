
pinput = None
t = Test()
t.command = "./program"
t.input = "input"
test 
r = run("./program -a 1 -b 2", pinput)

compareToFile(r.output, "stdout")
compareToFile(r.errout, "stderr")


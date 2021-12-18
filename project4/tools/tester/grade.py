def parse(reportPath):
   grade = { }
   fh = open(reportPath, "r")
   passed = -1
   total = -1
   for line in fh.readlines():
      words = line.split()
      if len(words) < 1:
         continue
      if words[0].lower() == "test" and len(words) == 3:
         grade[words[1]] = words[2]
      if words[0] == "Overall":
         passed = words[1]
         total = words[3]
   return (grade, passed, total)


from subprocess import *
from threading import Timer
import shlex, os, glob, difflib
import sys, string, signal

def addcslashes(s):
   special = {
      "\a":"\\a",
      "\r":"\\r",
      "\f":"\\f",      
      "\n":"\\n",
      "\t":"\\t",
      "\v":"\\v",
      "\"":"\\\"",
      "\\":"\\\\"
      }
   r = ""
   for char in s:
      if char in special:
         r += special[char]
      elif char in string.printable:
         r += char
      else:
         r += "\\" + "{0:o}".format(ord(char))
   return r

""" read an entire file into a string """
def readall(filename):
   f = open(filename, "rb")
   r = f.read()
   f.close()
   return r

def diff(expected, got):
   length = min(expected, 30)
   if len(got) > length*10:
      got = got[:length*5]
   s = ""
   s += "Expected: \"" + addcslashes(expected) + "\"\n"
   s += "Got:      \"" + addcslashes(got) + "\"\n"
   
   return s
   #diff = difflib.context_diff(expected.splitlines(1), got.splitlines(1), "expected", "got")
   #diff = difflib.unified_diff([expected], [got], "expected", "got")
   #diff = difflib.ndiff(expected.splitlines(1), got.splitlines(1))

   #for line in diff:
   #   sys.stdout.write(line)  # doctest: +NORMALIZE_WHITESPACE
   #sys.stdout.writelines(diff)
   return "".join(diff)


class Failure:
   def __init__(self, reason, explination=None):
      self.reason = reason
      self.explination = explination
   
   def __str__(self):
      if self.explination is not None:
         return self.reason + "\n" + str(self.explination) + "\n"
      else:
         return self.reason + "\n"

DEFAULT_TIME_LIMIT = 10

class Test:

   def __init__(self, name=None):
      self.name = name
      self.failures = list()
      self.warnings = list()
      self.child = None
      self.timelimit = DEFAULT_TIME_LIMIT
      self.failed = False
      self.fatal = False
      self.command = None
      self.stderrdata = None
      self.stdoutdata = None

   def fail(self, reason, explination=None):
      self.failures.append(Failure(reason, explination))
      self.failed = True

   def warn(self, reason, explination=None):
      self.warnings.append(Failure(reason, explination))

   def checkRequiredFile(self, path, msg=None):
      if not os.path.exists(path):
         self.fail("Required file '" + path + "' not found.", msg)
         return False
      return True

   def assertCreateFile(self, path, readable=False):
      if not os.path.exists(path):
         self.fail("Project failed to create file '" + path + "'.")
         return False

      if readable:
         try:
            open(path, "r").close()
         except IOError as (errno, strerror):
            self.fail("Could not read file '" + path + "'.")
         return False

      return True


   def removeFiles(self, paths, backup=True):
      for path in paths:
         if os.path.exists(path):
            if backup:
               backupPath = path + "~"
               if os.path.exists(backupPath):
                  os.remove(backupPath)
               os.rename(path, backupPath)
            else:
               os.remove(path)


   def timeout(self):
      if self.child is not None:
         self.child.kill()
      self.fail("Timelimit " + str(self.timelimit) + "s "
            + "exceeded, program is assumed to be stuck.")

   """Run a command with a timeline and input, store stdout and stderr"""
   def run(self, command, stdindata=None, inputFile=None, env=None,
         timelimit=None):
      
      self.command = command
      args = shlex.split(command)

      if not os.path.exists(args[0]):
         self.fail("Could not run '" + command + "', does not exist.", 
         "Build the project before running this test.")
         return False

      killtimer = None

      if timelimit is not None:
         self.timelimit = timelimit

      if self.timelimit > 0:
         self.timelimt = timelimit
         killtimer = Timer(self.timelimit, self.timeout)
         killtimer.start()

      # run program, wait for it to finish and get its stdout and stderr
      self.child = Popen(args, stdin=PIPE, stdout=PIPE, stderr=PIPE, env=env)
      (self.stdoutdata, self.stderrdata) = self.child.communicate(stdindata)

      if killtimer is not None:
         killtimer.cancel()


   """Run a command with a timeline and input, store stdout and stderr"""
   def runTestProg(self, command, target, stdindata=None, inputFile=None, env=None,
         timelimit=None, gdb=False):

      self.command = command
      args = shlex.split(command)

      if gdb:
         call(["gdb", "-symbols", target, "--args"] + args);
         self.fail("Cannot run test with gdb option")
         return True;

      if not os.path.exists(args[0]):
         self.fail("Could not run '" + command + "', does not exist.")
         return False

      killtimer = None

      if timelimit is not None:
         self.timelimit = timelimit

      if self.timelimit > 0:
         self.timelimt = timelimit
         killtimer = Timer(self.timelimit, self.timeout)
         killtimer.start()

      # run program, wait for it to finish and get its stdout and stderr
      self.child = Popen(args, stdin=PIPE, stdout=PIPE, stderr=PIPE, env=env)
      (self.stdoutdata, self.stderrdata) = self.child.communicate(stdindata)

      if killtimer is not None:
         killtimer.cancel()

      if self.child.returncode != 0:
         if self.child.returncode == -signal.SIGSEGV:
            self.fail("Segmentation fault")
         self.fail(self.stdoutdata + self.stderrdata)
         return False

      return True
   

   """Remove tagets and object files to ensure make works"""
   def cleanProject(self, targets):
      self.child = Popen(["make", "clean"], stdout=PIPE, stderr=STDOUT)
      self.stdoutdata = self.child.communicate()[0]

      garbage = [target for target in targets if os.path.exists(target)]
      garbage.extend(glob.glob("*.o"))

      if len(garbage) > 0:
         self.warn("Object files remain after 'make clean'.",
               "Files: " + (" ").join(garbage))
         self.removeFiles(garbage)         


   """Run make to try and build project"""
   def makeProject(self):
      ret = True
      self.child = Popen(["make"], stdout=PIPE, stderr=STDOUT)
      self.stdoutdata = self.child.communicate()[0]

      if self.child.returncode != 0:
         self.fail("Make returned an error.", self.stdoutdata)
         ret = False
      
      return ret

   def buildFallback(self, target):
      call(["make", target])
      if os.path.exists(target):
         return True
      
      call(["gcc", "-o", target] + glob.glob("*.c"))
      if os.path.exists(target):
         return True

      call(["gcc", "-o", target, target + ".c"])
      if os.path.exists(target):
         return True

      return False


   def assertExitStatus(self, expected):
      if self.child is None:
         self.fail("Project was not run, no exit status.")
         return False
      if self.child.returncode != expected:
         self.fail("Wrong exit status.", 
               "Expected " + str(expected) + 
               ", got " + str(self.child.returncode))
         return False
      return True
   
   def assertEqualStrings(self, msg, expected, got):
      if got != expected:
         self.fail(msg, diff(expected, got))
         return False
      return True

   def assertEqualValues(self, name, expected, got):
      if got != expected:
         self.fail("inccorrect value for '" + name + "' " + 
               "expected: " + str(expected) + " got: " + str(got))
         return False
      return True
   
   def assertStdout(self, expected):
      if self.child is None:
         self.fail("Project was not run, no stdout.")
         return False
      return self.assertEqualStrings("Wrong stdout.", expected, self.stdoutdata)

   def assertStderr(self, expected):
      if self.child is None:
         self.fail("Project was not run, no stderr.")
         return False
      return self.assertEqualStrings("Wrong stderr.", expected, self.stderrdata)

   def assertNoErrors(self):
      self.assertExitStatus(0)
      self.assertStderr("")

   def assertEqualFiles(self, expected, got):
      if self.assertCreateFile(got, readable=True):
         expectedData = readall(expected)
         gotData = readall(got)
         if gotData != expectedData:
            self.fail("Wrong data in file '" + got + "'.", 
                  diff(expectedData, gotData))
            return False
         return True
      return False


   def __str__(self):
      s = ""
      if self.failed and self.command is not None:
         s += self.command + "\n"
      for failure in self.failures:
         s += str(failure)
      for warning in self.warnings:
         s += str(warning)
      return s




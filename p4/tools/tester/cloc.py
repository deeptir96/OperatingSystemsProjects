import subprocess
from xml.dom import minidom
clocPath = "/u/c/s/cs537-1/ta/tools/cloc"
astylePath = "/u/c/s/cs537-1/ta/tools/astyle"


from glob import glob
import tempfile, os, shutil

class cloc:
   def __init__(self, path):
      #print [clocPath, path]
      report = subprocess.Popen([clocPath, "--quiet", "--xml", path], 
            stdout=subprocess.PIPE).communicate()[0]
      if report.count("<") == 0:
         self.sum_files = 0
         self.blank = 0
         self.comment = 0
         self.code = 0
         return
      index = report.index("<")
      #print report[index:]
      doc = minidom.parseString(report[index:])
      #print doc
      totals = doc.getElementsByTagName("total")[0]
      self.sum_files = int(totals.getAttribute("sum_files"))
      self.blank = int(totals.getAttribute("blank"))
      self.comment = int(totals.getAttribute("comment"))
      self.code = int(totals.getAttribute("code"))

def normalize(path):
   srcfiles = glob(path + "/" + "*.c") + glob(path + "/" + "*.h")
   temp = tempfile.mkdtemp()
   for f in srcfiles:
      shutil.copy(path + "/" + f, temp)
   files = glob(temp + "/" + "*.c") + glob(temp + "/" + "*.h")
   if len(files) > 0:
      subprocess.call(["indent", "-br", "-ce", "-cdw", "-i3", "--no-tabs"] + files)
      subprocess.call([clocPath, 
         "--strip-comments=nc", 
         "--original-dir",
         "--quiet"]
         + files)
   for f in files:
      os.rename(f + ".nc", f)
   return temp

def wc(path):
   words = 0
   for f in path:
      print f
      fd = open(f)
      for line in fd.readlines():
         words += len(line.split())
   return words


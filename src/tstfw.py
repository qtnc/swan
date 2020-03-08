from subprocess import PIPE, Popen
from io import StringIO
import sys

class SwanProc:
	def __init__ (self):
		self.proc = None
	
	def __enter__ (self):
		if self.proc: return
		self.proc = Popen(args='swand', stdin=PIPE, stderr=PIPE, stdout=PIPE, encoding='utf-8')
		self.proc.__enter__()
		self.read()
		return self
	
	def __exit__ (self, *args):
		if not self.proc: return
		self.send('quit')
		self.proc.wait(timeout=3)
		self.proc.__exit__(*args)
		self.proc = None
	
	def read (self):
		if not self.proc: return
		_in = self.proc.stdout
		out = StringIO()
		while True:
			c = _in.read(1)
			out.write(c)
			if c=='>':
				s = out.getvalue()
				if s.endswith('?>>'): return s[:-3].strip()
	
	def send (self, s):
		if not self.proc: return
		out = self.proc.stdin
		out.write(s)
		out.write('\n')
		out.flush()


passed = 0
failed = 0
total = 0
sent = ''
expected = ''
obtained = ''

with open(sys.argv[1], 'r', encoding='utf-8') as file:
	with SwanProc() as proc:
		for line in file:
			line = line.strip()
			if not line or line=='' or line.startswith('#'): continue
			elif line.startswith('>'):
				line = line[1:].strip()
				proc.send(line)
				sent += line
			else:
				expected = line
				obtained = proc.read()
				total+=1
				if obtained==expected: passed+=1
				else:
					failed+=1
					print("Failed: {}\r\nObtained: {}\r\nExpected: {}\r\n" .format(sent, obtained, expected))
				sent=''


if failed>0:
	print("{} tests, {} passed, {} failed" .format(total, passed, failed))
	sys.exit(1)

from Trick import Trick

class ExitStatus(Trick): 
	def __init__(self, options):
		pass
	
	def exit(self, pid, exitstatus, signal):
		if signal:
			print '### [%s] exited signal = %s ###' % (pid, signal)
		else:
			print '### [%s] exited status = %s ###' % (pid, exitstatus)
			
	def callmask(self):
		return {}


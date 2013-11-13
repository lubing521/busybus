# Copyright (C) 2013 Bartosz Golaszewski <bartekgola@gmail.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation,
# Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

"""
This module contains functions and data structures for easy creating
of regression tests for the Busybus.
"""

import sys
import subprocess
import re

binaries = {'bbusd' : './bbusd',
		'echod' : './bbus-echod',
		'call' : './bbus-call'}

scenDir = './test/regression/scenarios'
pyFileRegex = re.compile('^.+\.py$')

def printmsg(msg, header, err=False):
	if err:
		sys.stderr.write(header)
		sys.stderr.write(msg)
		sys.stderr.write('\n')
	else:
		sys.stdout.write(header)
		sys.stdout.write(msg)
		sys.stdout.write('\n')

def printinfo(msg):
	printmsg(msg, '[INFO]\t')

def printwarn(msg):
	printmsg(msg, '[WARN]\t\t')

def printerr(msg):
	printmsg(msg, '[ERROR]\t\t', True)

class ScenarioErr(Exception):
	"""
	Used to indicate, that a test scenario has failed.
	"""

	def __init__(self, msg):
		Exception.__init__(self, msg)
		self.scenario = None

	def setScenario(self, scenario):
		self.scenario = scenario

	def __repr__(self):
		return 'Scenario \'{0}\' failed: {1}'.format(
					self.scenario, repr(self.message))

class Process(subprocess.Popen):
	"""
	Used to start and stop external processes.
	"""

	def __init__(self, prog, args=[]):
		subprocess.Popen.__init__(self, [prog] + args,
						shell=False,
						stdout=subprocess.PIPE,
						stderr=subprocess.PIPE)

	def __del__(self):
		try:
			if self.poll() is None:
				self.kill()
		except:
			pass  # Ignore exceptions in the destructor.

def isPyFile(file):
	return pyFileRegex.match(file)

def getBinPath(name):
	return binaries[name]

def regexMatch(pattern, string):
	return re.match(pattern, string)

def callExpect(prog, args=[], retcode=0, stdout='', stderr=''):
	"""
	Run program 'prog' with a list of arguments 'args' and expect it to
	return 'retcode' and to output strings matching the 'stdout' and
	'stderr' regular expressions on the corresponding output streams.
	If either the return value or program's output differ from expected
	the function will throw a ScenarioErr.
	"""

	proc = Process(getBinPath(prog), args)
	out = proc.communicate()
	proc.wait()  # TODO Some kind of timeout.

	if proc.returncode != retcode:
		raise ScenarioErr('Program \'{0}\' exited with status: ({1}), '
				'but expected exit code is ({2})'.format(
					prog, proc.returncode, retcode))

	if not regexMatch(stdout, out[0]) or not regexMatch(stderr, out[1]):
		raise ScenarioErr('Program output is: (stdout = \'{0}\','
			'stderr = \'{1}\'), but expected output is:'
			' (stdout = \'{2}\',' ' stderr = \'{3}\')'.format(
					out[0], out[1], stdout, stderr))



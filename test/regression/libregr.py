# Copyright (C) 2013 Bartosz Golaszewski
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

binaries = { 'bbusd' : './bbusd',
		'echod' : './bbus-echod',
		'call' : './bbus-call' }

scenDir = './test/regression/scenarios'
pyFileRegex = re.compile('.+\.py')

def printinfo(msg):
	sys.stdout.write('[INFO]\t')
	sys.stdout.write(msg)
	sys.stdout.write('\n')

def printerr(msg):
	sys.stdout.write('[ERROR]\t\t')
	sys.stdout.write(msg)
	sys.stdout.write('\n')

class ScenarioExc(Exception):
	"""
	Used to indicate, that a test scenario has failed.
	"""

	def __init__(self, case, msg):
		Exception.__init__(self, msg)
		self.case = case

class Process(subprocess.Popen):
	"""
	Used to start and stop external processes.
	"""

	def __init__(self, prog):
		subprocess.Popen.__init__(self, [ prog ], shell=False)

	def __del__(self):
		try:
			if self.poll() is None:
				self.kill()
		except:
			pass  # Ignore exceptions in destructor.

def isPyFile(file):
	return pyFileRegex.match(file)

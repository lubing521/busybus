#!/usr/bin/env python

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
Set of regression tests for the Busybus.
"""

import sys
import os
import libregr
import getopt
import time

class Options(object):
	"""
	Program run-time options.
	"""
	def __init__(self):
		self.debug = False
		self.cmd = ''
		self.cmdArgs = None

class ShowUsage(Exception):
	def __init__(self):
		Exception.__init__(self)

progName = ''
bbusd = None
scenFailed = []
scenRun = 0
options = Options()

def printUsage():
	sys.stdout.write('Usage:\n')
	sys.stdout.write('  {0} <command> [arguments]\n'.format(progName))
	sys.stdout.write('\n')
	sys.stdout.write('Commands:\n')
	sys.stdout.write('  help - display this message\n')
	sys.stdout.write('  run - run all tests\n')
	sys.stdout.write('  run [args] - run listed scenarios\n')
	sys.stdout.write('\n')

def init():
	global bbusd
	bbusd = libregr.Process(libregr.getBinPath('bbusd'))
	# FIXME Find a better way to wait for bbusd activation.
	time.sleep(1 / 1000.0)
	if bbusd.poll() is not None:
		out = bbusd.communicate()
		raise RuntimeError('Error starting bbusd{0}'.format(
					': ' + out[1] if out[1] else ''))

def printExitErr(prog, code):
	libregr.printerr(
		'Warning: \'{0}\' exited with error code: {1}'.format(
								prog, code))

def finalize():
	bbusd.terminate()
	if bbusd.wait() != 0:
		printExitErr('bbusd', bbusd.returncode)

def printScenErr(ex):
	libregr.printwarn(repr(ex))

def runScenario(scenarioName):
	global scenRun, scenFailed
	libregr.printinfo('Running scenario \'{0}\''.format(scenarioName))
	scenario = __import__(scenarioName)
	if hasattr(scenario, 'init'):
		scenario.init()
	try:
		scenario.run()
	except libregr.ScenarioErr as ex:
		ex.setScenario(scenarioName)
		printScenErr(ex)
		scenFailed.append(ex.scenario)
	finally:
		if hasattr(scenario, 'finalize'):
			scenario.finalize()
		del scenario
	scenRun = scenRun + 1

def runAll():
	libregr.printinfo('Running all regression scenarios')
	files = os.listdir(libregr.scenDir)
	for f in files:
		fullpath = '{0}/{1}'.format(libregr.scenDir, f)
		if os.path.isfile(fullpath) and libregr.isPyFile(f):
			runScenario(f[:-3])

def printResults():
	libregr.printinfo('Regression ended, {0} {1} run'.format(
				scenRun, 'scenario' if scenRun == 1
					else 'scenarios'))
	if scenFailed:
		numFailed = len(scenFailed)
		libregr.printwarn(
			'Following {0} {1} failed'.format(
				numFailed, 'test' if numFailed == 1
					else 'tests'))
		for s in scenFailed:
			libregr.printwarn('\t{0}'.format(s))
	else:
		libregr.printinfo('All tests successful')

def parseOps(argv):
	global progName, options
	progName = argv[0]
	shortopts = 'du'
	longopts = ('debug', 'usage')
	cmds = ('run')
	opts, args = getopt.getopt(argv[1:], shortopts, longopts)
	for o, a in opts:
		if o in ('-d', '--debug'):
			options.debug = True
		elif o in ('-u', '--usage'):
			raise ShowUsage()
		else:
			raise getopt.GetoptError('Unhandled option', o)
	if len(args) < 1:
		raise getopt.GetoptError('Command not specified')
	else:
		options.cmd = args[0]
		if len(args) > 1:
			options.cmdArgs = args[1:]

def main(argv):
	try:
		parseOps(argv)
	except getopt.GetoptError as ex:
		libregr.printerr('Error parsing arguments: {0}'.format(ex))
		return 1
	except ShowUsage:
		printUsage()
		return 0

	if options.cmd == 'run':
		libregr.printinfo('#######################################')
		libregr.printinfo('###> Busybus regression test suite <###')
		libregr.printinfo('#######################################')
		try:
			init();
			sys.path.append(os.path.abspath(libregr.scenDir))
			if not options.cmdArgs:
				runAll()
			else:
				for s in options.cmdArgs:
					runScenario(s)
			finalize()
			printResults()
		except Exception as ex:
			if options.debug:
				raise
			else:
				libregr.printerr(
					'Fatal regression error: '
						'{0}\n'.format(ex))
				return 1
	else:
		libregr.printerr('Logic error, this should not happen')
		return 1

	return 0

if __name__ == '__main__':
	sys.exit(main(sys.argv))


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
sys.dont_write_bytecode = True
import os
import libregr
import getopt
import time

progName = ''
bbusd = None
scenFailed = []

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
	bbusd = libregr.Process(libregr.binaries['bbusd'])
	# FIXME Find a better way to wait for bbusd activation.
	time.sleep(1 / 1000.0)
	if bbusd.poll() is not None:
		raise RuntimeError('Error starting bbusd')

def printExitErr(prog, code):
	sys.stderr.write(
		'Warning: \'{0}\' exited with error code: {1}\n'.format(
								prog, code))

def finalize():
	bbusd.terminate()
	if bbusd.wait() != 0:
		printExitErr(binaries[key], bbusd.returncode)

def printScenErr(ex):
	sys.stderr.write(
		'Scenarion \'{0}\' failed: {1}\n'.format(ex.case, ex.msg))

def runScenario(scenario):
	sys.stdout.write('Running scenario \'{0}\''.format(scenario))
	import scenario
	if hasattr(scenario, 'init'):
		scenario.init()
	try:
		scenario.run()
	except libregr.ScenarioExc as ex:
		printScenErr(ex)
		scenFailed.append(ex.case)
	if hasattr(scenario, 'finalize'):
		scenario.finalize()
	del scenario

def runAll():
	sys.stdout.write('Running all regression scenarios\n')
	files = os.listdir(libregr.scenDir)
	for f in files:
		if os.path.isfile(f) and libregr.isPyFile(f):
			runScenario(f[:-3])

def printResults():
	sys.stdout.write('Regression ended\n')
	if scenFailed:
		sys.stdout.write(
			'Following {0} tests failed\n'.format(len(scenFailed)))
		for s in scenFailed:
			sys.stdout.write('{0}\n'.format(s))
	else:
		sys.stdout.write('All tests successful\n')

def main(argv):
	global progName

	progName = argv[0]
	cmd = argv[1] if len(argv) > 1 else None
	if len(argv) > 1 and cmd == 'run':
		sys.stdout.write('Busybus regression test suite\n')
		try:
			init();
			sys.path.append(os.path.abspath(libregr.scenDir))
			if len(argv) == 2:
				runAll()
			else:
				for s in argv[2:]:
					runScenario(s)
			finalize()
			printResults()
		except Exception as ex:
			sys.stderr.write(
				'Fatal regression error: {0}\n'.format(ex))
			return 1
	else:
		printUsage()
		return 1

	return 0

if __name__ == '__main__':
	sys.exit(main(sys.argv))


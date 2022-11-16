#! /usr/bin/env python
# encoding: utf-8
# a1batross, mittorn, 2018

from __future__ import print_function
from waflib import Logs, Context, Configure
import sys
import os

VERSION = '2.4'
APPNAME = 'hlsdk-xash3d'
top = '.'

Context.Context.line_just = 55 # should fit for everything on 80x26

@Configure.conf
def get_taskgen_count(self):
	try: idx = self.tg_idx_count
	except: idx = 0 # don't set tg_idx_count to not increase counter
	return idx

def options(opt):
	grp = opt.add_option_group('Common options')

	grp.add_option('-8', '--64bits', action = 'store_true', dest = 'ALLOW64', default = False,
		help = 'allow targetting 64-bit engine(Linux/Windows/OSX x86 only) [default: %default]')

	grp.add_option('--enable-voicemgr', action = 'store_true', dest = 'VOICEMGR', default = False,
		help = 'enable voice manager [default: %default]')

	grp.add_option('--disable-goldsrc-support', action = 'store_false', dest = 'GOLDSRC', default = True,
		help = 'disable GoldSource engine support [default: %default]')

	opt.load('compiler_optimizations subproject')

	opt.add_subproject(['cl_dll', 'dlls'])

	opt.load('xcompile compiler_cxx compiler_c clang_compilation_database strip_on_install msdev msvs')
	if sys.platform == 'win32':
		opt.load('msvc')
	opt.load('reconfigure')

def configure(conf):
	# Configuration
	conf.env.GAMEDIR     = 'valve'
	conf.env.CLIENT_DIR  = 'cl_dlls'
	conf.env.SERVER_DIR  = 'dlls'
	conf.env.SERVER_NAME = 'hl'
	conf.env.PREFIX = ''

	conf.load('fwgslib reconfigure compiler_optimizations enforce_pic')

	enforce_pic = True # modern defaults

	conf.env.VOICEMGR    = conf.options.VOICEMGR
	conf.env.GOLDSRC     = conf.options.GOLDSRC

	# Force XP compability, all build targets should add
	# subsystem=bld.env.MSVC_SUBSYSTEM
	# TODO: wrapper around bld.stlib, bld.shlib and so on?
	conf.env.MSVC_SUBSYSTEM = 'WINDOWS,5.01'
	conf.env.MSVC_TARGETS = ['x86' if not conf.options.ALLOW64 else 'x64']

	# Load compilers early
	conf.load('xcompile compiler_c compiler_cxx')

	# HACKHACK: override msvc DEST_CPU value by something that we understand
	if conf.env.DEST_CPU == 'amd64':
		conf.env.DEST_CPU = 'x86_64'

	if conf.env.COMPILER_CC == 'msvc':
		conf.load('msvc_pdb')

	conf.load('msvs msdev strip_on_install')

	if conf.env.DEST_OS == 'android':
		conf.options.GOLDSRC = False
		conf.env.SERVER_NAME = 'server' # can't be any other name, until specified
	
	if conf.env.MAGX:
		enforce_pic = False

	conf.check_pic(enforce_pic)

	# We restrict 64-bit builds ONLY for Win/Linux/OSX running on Intel architecture
	# Because compatibility with original GoldSrc
	if conf.env.DEST_OS in ['win32', 'linux', 'darwin'] and conf.env.DEST_CPU in ['x86_64']:
		conf.env.BIT32_ALLOW64 = conf.options.ALLOW64
		if not conf.env.BIT32_ALLOW64:
			Logs.info('WARNING: will build engine for 32-bit target')
	else:
		conf.env.BIT32_ALLOW64 = True
	conf.env.BIT32_MANDATORY = not conf.env.BIT32_ALLOW64
	conf.load('force_32bit library_naming')

	compiler_optional_flags = [
		'-fdiagnostics-color=always',
		'-Werror=return-type',
		'-Werror=parentheses',
		'-Werror=vla',
		'-Werror=tautological-compare',
		'-Werror=duplicated-cond',
		'-Werror=bool-compare',
		'-Werror=bool-operation',
		'-Wstrict-aliasing',
	]

	c_compiler_optional_flags = [
		'-Werror=implicit-function-declaration',
		'-Werror=int-conversion',
		'-Werror=implicit-int',
		'-Werror=declaration-after-statement'
	]

	cflags, linkflags = conf.get_optimization_flags()

	# Here we don't differentiate C or C++ flags
	if conf.options.LTO:
		lto_cflags = {
			'msvc':  ['/GL'],
			'gcc':   ['-flto'],
			'clang': ['-flto']
		}

		lto_linkflags = {
			'msvc':  ['/LTCG'],
			'gcc':   ['-flto'],
			'clang': ['-flto']
		}
		cflags    += conf.get_flags_by_compiler(lto_cflags, conf.env.COMPILER_CC)
		linkflags += conf.get_flags_by_compiler(lto_linkflags, conf.env.COMPILER_CC)

	if conf.options.POLLY:
		polly_cflags = {
			'gcc':   ['-fgraphite-identity'],
			'clang': ['-mllvm', '-polly']
			# msvc sosat :(
		}

		cflags   += conf.get_flags_by_compiler(polly_cflags, conf.env.COMPILER_CC)

	# And here C++ flags starts to be treated separately
	cxxflags = list(cflags)
	if conf.env.COMPILER_CC != 'msvc':
		conf.check_cc(cflags=cflags, msg= 'Checking for required C flags')
		conf.check_cxx(cxxflags=cflags, msg= 'Checking for required C++ flags')

		cflags += conf.filter_cflags(compiler_optional_flags + c_compiler_optional_flags, cflags)
		cxxflags += conf.filter_cxxflags(compiler_optional_flags, cflags)

	conf.env.append_unique('CFLAGS', cflags)
	conf.env.append_unique('CXXFLAGS', cxxflags)
	conf.env.append_unique('LINKFLAGS', linkflags)

	# check if we can use C99 tgmath
	if conf.check_cc(header_name='tgmath.h', mandatory=False):
		tgmath_usable = conf.check_cc(fragment='''#include<tgmath.h>
			int main(void){ return (int)sin(2.0f); }''',
			msg='Checking if tgmath.h is usable', mandatory=False)
		conf.define_cond('HAVE_TGMATH_H', tgmath_usable)
	else:
		conf.undefine('HAVE_TGMATH_H')
	cmath_usable = conf.check_cxx(fragment='''#include<cmath>
			int main(void){ return (int)sqrt(2.0f); }''',
			msg='Checking if cmath is usable', mandatory = False)
	conf.define_cond('HAVE_CMATH', cmath_usable)

	if conf.env.COMPILER_CC == 'msvc':
		conf.define('_CRT_SECURE_NO_WARNINGS', True)
		conf.define('_CRT_NONSTDC_NO_DEPRECATE', True)
	elif conf.env.COMPILER_CC == 'owcc':
		pass
	else:
		conf.env.append_unique('DEFINES', ['stricmp=strcasecmp', 'strnicmp=strncasecmp', '_snprintf=snprintf', '_vsnprintf=vsnprintf', '_LINUX', 'LINUX'])
		conf.env.append_unique('CXXFLAGS', ['-Wno-invalid-offsetof', '-fno-rtti', '-fno-exceptions'])

	# strip lib from pattern
	if conf.env.DEST_OS not in ['android']:
		if conf.env.cshlib_PATTERN.startswith('lib'):
			conf.env.cshlib_PATTERN = conf.env.cshlib_PATTERN[3:]
		if conf.env.cxxshlib_PATTERN.startswith('lib'):
			conf.env.cxxshlib_PATTERN = conf.env.cxxshlib_PATTERN[3:]

	conf.define('BARNACLE_FIX_VISIBILITY', False)
	conf.define('CLIENT_WEAPONS', True)
	conf.define('CROWBAR_IDLE_ANIM', False)
	conf.define('CROWBAR_DELAY_FIX', False)
	conf.define('CROWBAR_FIX_RAPID_CROWBAR', False)
	conf.define('GAUSS_OVERCHARGE_FIX', False)
	conf.define('OEM_BUILD', False)
	conf.define('HLDEMO_BUILD', False)

	conf.add_subproject(["cl_dll", "dlls"])

def build(bld):
	bld.add_subproject(["cl_dll", "dlls"])




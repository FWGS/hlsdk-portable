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

	grp.add_option('-T', '--build-type', action='store', dest='BUILD_TYPE', default = None,
		help = 'build type: debug, release or none(custom flags)')

	grp.add_option('-8', '--64bits', action = 'store_true', dest = 'ALLOW64', default = False,
		help = 'allow targetting 64-bit engine(Linux/Windows/OSX x86 only) [default: %default]')

	grp.add_option('--enable-voicemgr', action = 'store_true', dest = 'VOICEMGR', default = False,
		help = 'enable voice manager [default: %default]')

	grp.add_option('--enable-goldsrc-support', action = 'store_true', dest = 'GOLDSRC', default = False,
		help = 'enable GoldSource engine support [default: %default]')

	grp.add_option('--enable-lto', action = 'store_true', dest = 'LTO', default = False,
		help = 'enable Link Time Optimization [default: %default]')

	grp.add_option('--enable-poly-opt', action = 'store_true', dest = 'POLLY', default = False,
		help = 'enable polyhedral optimization if possible [default: %default]')

	grp.add_option('--enable-magx', action = 'store_true', dest = 'MAGX', default = False,
		help = 'enable targetting for MotoMAGX phones [default: %default]')

	grp.add_option('--enable-simple-mod-hacks', action = 'store_true', dest = 'ENABLE_MOD_HACKS', default = False,
		help = 'enable hacks for simple mods that mostly compatible with Half-Life but has little changes. Enforced for Android. [default: %default]')

	opt.load('subproject')

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

	conf.load('fwgslib reconfigure enforce_pic')

	enforce_pic = True # modern defaults
	valid_build_types = ['fastnative', 'fast', 'release', 'debug', 'nooptimize', 'sanitize', 'none']
	conf.load('fwgslib reconfigure')
	conf.start_msg('Build type')
	if conf.options.BUILD_TYPE == None:
		conf.end_msg('not set', color='RED')
		conf.fatal('Please set a build type, for example "-T release"')
	elif not conf.options.BUILD_TYPE in valid_build_types:
		conf.end_msg(conf.options.BUILD_TYPE, color='RED')
		conf.fatal('Invalid build type. Valid are: %s' % ', '.join(valid_build_types))
	conf.end_msg(conf.options.BUILD_TYPE)

	# -march=native should not be used
	if conf.options.BUILD_TYPE == 'fast':
		Logs.warn('WARNING: \'fast\' build type should not be used in release builds')

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

	try:
		conf.env.CC_VERSION[0]
	except IndexError:
		conf.env.CC_VERSION = (0, )

	if conf.env.DEST_OS == 'android':
		conf.options.GOLDSRC = False
		conf.env.SERVER_NAME = 'server' # can't be any other name, until specified
	
	conf.env.MAGX = conf.options.MAGX
	if conf.options.MAGX:
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

	linker_flags = {
		'common': {
			'msvc':  ['/DEBUG'], # always create PDB, doesn't affect result binaries
			'gcc':   ['-Wl,--no-undefined']
		},
		'sanitize': {
			'clang': ['-fsanitize=undefined', '-fsanitize=address'],
			'gcc':   ['-fsanitize=undefined', '-fsanitize=address'],
		}
	}

	compiler_c_cxx_flags = {
		'common': {
			# disable thread-safe local static initialization for C++11 code, as it cause crashes on Windows XP
			'msvc':    ['/D_USING_V110_SDK71_', '/Zi', '/FS', '/Zc:threadSafeInit-', '/MT'],
			'clang':   ['-g', '-gdwarf-2', '-fvisibility=hidden'],
			'gcc':     ['-g']
		},
		'fast': {
			'msvc':    ['/O2', '/Oy'],
			'gcc': {
				'3':       ['-O3', '-Os', '-funsafe-math-optimizations', '-fomit-frame-pointer'],
				'default': ['-Ofast', '-funsafe-math-optimizations', '-funsafe-loop-optimizations', '-fomit-frame-pointer']
			},
			'clang':   ['-Ofast'],
			'default': ['-O3']
		},
		'fastnative': {
			'msvc':    ['/O2', '/Oy'],
			'gcc':     ['-Ofast', '-march=native', '-funsafe-math-optimizations', '-funsafe-loop-optimizations', '-fomit-frame-pointer'],
			'clang':   ['-Ofast', '-march=native'],
			'default': ['-O3']
		},
		'release': {
			'msvc':    ['/O2'],
			'default': ['-O3']
		},
		'debug': {
			'msvc':    ['/O1'],
			'gcc':     ['-Og'],
			'default': ['-O1']
		},
		'sanitize': {
			'msvc':    ['/Od', '/RTC1'],
			'gcc':     ['-Og', '-fsanitize=undefined', '-fsanitize=address'],
			'clang':   ['-O0', '-fsanitize=undefined', '-fsanitize=address'],
			'default': ['-O0']
		},
		'nooptimize': {
			'msvc':    ['/Od'],
			'default': ['-O0']
		}
	}

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

	linkflags = conf.get_flags_by_type(linker_flags, conf.options.BUILD_TYPE, conf.env.COMPILER_CC, conf.env.CC_VERSION[0])
	cflags    = conf.get_flags_by_type(compiler_c_cxx_flags, conf.options.BUILD_TYPE, conf.env.COMPILER_CC, conf.env.CC_VERSION[0])

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
		conf.define('_CRT_SECURE_NO_WARNINGS', 1)
		conf.define('_CRT_NONSTDC_NO_DEPRECATE', 1)
	elif conf.env.COMPILER_CC == 'owcc':
		pass
	else:
		conf.env.append_unique('DEFINES', ['stricmp=strcasecmp', 'strnicmp=strncasecmp', '_snprintf=snprintf', '_vsnprintf=vsnprintf', '_LINUX', 'LINUX'])
		conf.env.append_unique('CXXFLAGS', ['-Wno-invalid-offsetof', '-fno-rtti', '-fno-exceptions'])

	# strip lib from pattern
	if conf.env.DEST_OS in ['linux', 'darwin']:
		if conf.env.cshlib_PATTERN.startswith('lib'):
			conf.env.cshlib_PATTERN = conf.env.cshlib_PATTERN[3:]
		if conf.env.cxxshlib_PATTERN.startswith('lib'):
			conf.env.cxxshlib_PATTERN = conf.env.cxxshlib_PATTERN[3:]

	conf.define('CLIENT_WEAPONS', '1')
	conf.define('CROWBAR_IDLE_ANIM', False)
	conf.define('CROWBAR_DELAY_FIX', False)
	conf.define('CROWBAR_FIX_RAPID_CROWBAR', False)
	conf.define('GAUSS_OVERCHARGE_FIX', False)

	if conf.env.DEST_OS == 'android' or conf.options.ENABLE_MOD_HACKS:
		conf.define('MOBILE_HACKS', '1')

	conf.add_subproject(["cl_dll", "dlls"])

def build(bld):
	bld.add_subproject(["cl_dll", "dlls"])




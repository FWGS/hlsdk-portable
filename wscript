#! /usr/bin/env python
# encoding: utf-8
# a1batross, mittorn, 2018

from __future__ import print_function
from waflib import Logs
import sys
import os
sys.path.append(os.path.realpath('scripts/waflib'))

VERSION = '2.4'
APPNAME = 'hlsdk-xash3d'
top = '.'

def options(opt):
	grp = opt.add_option_group('Common options')

	grp.add_option('-T', '--build-type', action='store', dest='BUILD_TYPE', default = None,
		help = 'build type: debug, release or none(custom flags)')

	grp.add_option('-8', '--64bits', action = 'store_true', dest = 'ALLOW64', default = False,
		help = 'allow targetting 64-bit game dlls')

	grp.add_option('--enable-voicemgr', action = 'store_true', dest = 'VOICEMGR', default = False,
		help = 'enable voice manager')

	grp.add_option('--enable-goldsrc-support', action = 'store_true', dest = 'GOLDSRC', default = False,
		help = 'enable GoldSource engine support')

	opt.recurse('cl_dll dlls')

	opt.load('xcompile compiler_cxx compiler_c clang_compilation_database strip_on_install')
	if sys.platform == 'win32':
		opt.load('msvc msdev')
	opt.load('reconfigure')


def configure(conf):
	# Configuration
	conf.env.GAMEDIR     = 'valve'
	conf.env.CLIENT_DIR  = 'cl_dlls'
	conf.env.SERVER_DIR  = 'dlls'
	conf.env.SERVER_NAME = 'hl'
	conf.env.PREFIX = ''

	conf.load('reconfigure')

	conf.start_msg('Build type')
	if conf.options.BUILD_TYPE == None:
		conf.end_msg('not set', color='RED')
		conf.fatal('Please set a build type, for example "-T release"')
	elif not conf.options.BUILD_TYPE in ['fast', 'release', 'debug', 'nooptimize', 'sanitize', 'none']:
		conf.end_msg(conf.options.BUILD_TYPE, color='RED')
		conf.fatal('Invalid build type. Valid are "debug", "release" or "none"')
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
	conf.env.MSVC_TARGETS = ['x86'] # explicitly request x86 target for MSVC
	if sys.platform == 'win32':
		conf.load('msvc msdev')
	conf.load('xcompile compiler_c compiler_cxx strip_on_install')

	if conf.env.DEST_OS == 'android':
		conf.options.ALLOW64 = True
		conf.options.GOLDSRC = False
		conf.env.SERVER_NAME = 'server' # can't be any other name, until specified

	# print(conf.options.ALLOW64)

	conf.env.BIT32_MANDATORY = not conf.options.ALLOW64
	conf.env.BIT32_ALLOW64 = conf.options.ALLOW64
	conf.load('force_32bit')

	if conf.env.DEST_SIZEOF_VOID_P == 4:
		Logs.info('NOTE: will build game dlls for 32-bit target')
	else:
		Logs.warn('WARNING: 64-bit game dlls may be unstable')

	linker_flags = {
		'common': {
			'msvc':	   ['/DEBUG'], # always create PDB, doesn't affect result binaries
			'gcc':	   ['-Wl,--no-undefined']
		},
		'sanitize': {
			'gcc':	   ['-fsanitize=undefined', '-fsanitize=address'],
		}
	}

	compiler_c_cxx_flags = {
		'common': {
			'msvc':	   ['/D_USING_V110_SDK71_', '/Zi', '/FS'],
			'clang':   ['-g', '-gdwarf-2'],
			'gcc':	   ['-g', '-Werror=implicit-function-declaration', '-fdiagnostics-color=always']
		},
		'fast': {
			'msvc':	   ['/O2', '/Oy'], #todo: check /GL /LTCG
			'gcc':	   ['-Ofast', '-march=native', '-funsafe-math-optimizations', '-funsafe-loop-optimizations', '-fomit-frame-pointer'],
			'default': ['-O3']
		},
		'release': {
			'msvc':	   ['/O2'],
			'default': ['-O3']
		},
		'debug': {
			'msvc':	   ['/O1'],
			'gcc':	   ['-Og'],
			'default': ['-O1']
		},
		'sanitize': {
			'msvc':	   ['/Od', '/RTC1'],
			'gcc':	   ['-Og', '-fsanitize=undefined', '-fsanitize=address'],
			'default': ['-O1']
		},
		'nooptimize': {
			'msvc':	   ['/Od'],
			'default': ['-O0']
		}
	}

	conf.env.append_unique('CFLAGS', conf.get_flags_by_type(
	    compiler_c_cxx_flags, conf.options.BUILD_TYPE, conf.env.COMPILER_CC))
	conf.env.append_unique('CXXFLAGS', conf.get_flags_by_type(
	    compiler_c_cxx_flags, conf.options.BUILD_TYPE, conf.env.COMPILER_CC))
	conf.env.append_unique('LINKFLAGS', conf.get_flags_by_type(
	    linker_flags, conf.options.BUILD_TYPE, conf.env.COMPILER_CC))

	if conf.env.COMPILER_CC == 'msvc':
		conf.env.append_unique('DEFINES', ['_CRT_SECURE_NO_WARNINGS','_CRT_NONSTDC_NO_DEPRECATE'])
	else:
		conf.env.append_unique('DEFINES', ['stricmp=strcasecmp','strnicmp=strncasecmp','_LINUX','LINUX','_snprintf=snprintf','_vsnprintf=vsnprintf'])
		cflags = ['-fvisibility=hidden','-Wno-write-strings']
		conf.env.append_unique('CFLAGS', cflags)
		conf.env.append_unique('CXXFLAGS', cflags + ['-Wno-invalid-offsetof', '-fno-rtti', '-fno-exceptions'])

	# strip lib from pattern
	if conf.env.DEST_OS in ['linux', 'darwin']:
		if conf.env.cshlib_PATTERN.startswith('lib'):
			conf.env.cshlib_PATTERN = conf.env.cshlib_PATTERN[3:]
		if conf.env.cxxshlib_PATTERN.startswith('lib'):
			conf.env.cxxshlib_PATTERN = conf.env.cxxshlib_PATTERN[3:]

	conf.env.append_unique('DEFINES', 'CLIENT_WEAPONS')

	conf.recurse('cl_dll dlls')

def build(bld):
	bld.recurse('cl_dll dlls')
		
		
	

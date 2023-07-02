#! /usr/bin/env python
# encoding: utf-8
# a1batross, mittorn, 2018

from waflib import Build, Configure, Context, Logs
import sys
import os
import re

VERSION = '2.4'
APPNAME = 'hlsdk-portable'
top = '.'
default_prefix = '/'

Context.Context.line_just = 60 # should fit for everything on 80x26

@Configure.conf
def get_taskgen_count(self):
	try: idx = self.tg_idx_count
	except: idx = 0 # don't set tg_idx_count to not increase counter
	return idx

def options(opt):
	opt.load('reconfigure compiler_optimizations xcompile compiler_cxx compiler_c clang_compilation_database strip_on_install msdev msvs msvc subproject')

	grp = opt.add_option_group('Common options')

	grp.add_option('-8', '--64bits', action = 'store_true', dest = 'ALLOW64', default = False,
		help = 'allow targetting 64-bit engine(Linux/Windows/OSX x86 only) [default: %default]')
	grp.add_option('--disable-werror', action = 'store_true', dest = 'DISABLE_WERROR', default = False,
		help = 'disable compilation abort on warning')
	grp.add_option('--enable-voicemgr', action = 'store_true', dest = 'USE_VOICEMGR', default = False,
		help = 'Enable VOICE MANAGER')

	opt.add_subproject('dlls')
	opt.add_subproject('cl_dll')

def configure(conf):
	conf.load('fwgslib reconfigure compiler_optimizations')
	conf.env.MSVC_TARGETS = ['x86' if not conf.options.ALLOW64 else 'x64']

	# Force XP compatibility, all build targets should add subsystem=bld.env.MSVC_SUBSYSTEM
	if conf.env.MSVC_TARGETS[0] == 'x86':
		conf.env.MSVC_SUBSYSTEM = 'WINDOWS,5.01'
	else:
		conf.env.MSVC_SUBSYSTEM = 'WINDOWS'

	# Load compilers early
	conf.load('xcompile compiler_c compiler_cxx')

	# HACKHACK: override msvc DEST_CPU value by something that we understand
	if conf.env.DEST_CPU == 'amd64':
		conf.env.DEST_CPU = 'x86_64'

	if conf.env.COMPILER_CC == 'msvc':
		conf.load('msvc_pdb')

	conf.load('msvs msdev subproject clang_compilation_database strip_on_install enforce_pic')

	enforce_pic = True # modern defaults
	conf.check_pic(enforce_pic)

	# We restrict 64-bit builds ONLY for Win/Linux/OSX running on Intel architecture
	# Because compatibility with original GoldSrc
	if conf.env.DEST_OS in ['win32', 'linux', 'darwin'] and conf.env.DEST_CPU == 'x86_64':
		conf.env.BIT32_MANDATORY = not conf.options.ALLOW64
		if conf.env.BIT32_MANDATORY:
			Logs.info('WARNING: will build game for 32-bit target')
	else:
		conf.env.BIT32_MANDATORY = False

	conf.load('force_32bit')

	cflags, linkflags = conf.get_optimization_flags()
	cxxflags = list(cflags) # optimization flags are common between C and C++ but we need a copy

	# on the Switch, allow undefined symbols by default, which is needed for libsolder to work
	# we'll specifically disallow them for the engine executable
	# additionally, shared libs are linked without standard libs, we'll add those back in the engine wscript
	if conf.env.DEST_OS == 'nswitch':
		linkflags.remove('-Wl,--no-undefined')
		conf.env.append_unique('LINKFLAGS_cshlib', ['-nostdlib', '-nostartfiles'])
		conf.env.append_unique('LINKFLAGS_cxxshlib', ['-nostdlib', '-nostartfiles'])
	# same on the vita
	elif conf.env.DEST_OS == 'psvita':
		conf.env.append_unique('CFLAGS_cshlib', ['-fPIC'])
		conf.env.append_unique('CXXFLAGS_cxxshlib', ['-fPIC', '-fno-use-cxa-atexit'])
		conf.env.append_unique('LINKFLAGS_cshlib', ['-nostdlib', '-Wl,--unresolved-symbols=ignore-all'])
		conf.env.append_unique('LINKFLAGS_cxxshlib', ['-nostdlib', '-Wl,--unresolved-symbols=ignore-all'])
	# check if we need to use irix linkflags
	elif conf.env.DEST_OS == 'irix' and conf.env.COMPILER_CC == 'gcc':
		linkflags.remove('-Wl,--no-undefined')
		linkflags.append('-Wl,--unresolved-symbols=ignore-all')
		# check if we're in a sgug environment
		if 'sgug' in os.environ['LD_LIBRARYN32_PATH']:
			linkflags.append('-lc')

	conf.check_cc(cflags=cflags, linkflags=linkflags, msg='Checking for required C flags')
	conf.check_cxx(cxxflags=cxxflags, linkflags=linkflags, msg='Checking for required C++ flags')

	conf.env.append_unique('CFLAGS', cflags)
	conf.env.append_unique('CXXFLAGS', cxxflags)
	conf.env.append_unique('LINKFLAGS', linkflags)

	if conf.env.COMPILER_CC != 'msvc' and not conf.options.DISABLE_WERROR:
		opt_flags = [
			# '-Wall', '-Wextra', '-Wpedantic',
			'-fdiagnostics-color=always',

			# stable diagnostics, forced to error, sorted
			'-Werror=bool-compare',
			'-Werror=bool-operation',
			'-Werror=cast-align=strict',
			'-Werror=duplicated-cond',
			# '-Werror=format=2',
			'-Werror=implicit-fallthrough=2',
			# '-Werror=logical-op',
			'-Werror=packed',
			'-Werror=packed-not-aligned',
			'-Werror=parentheses',
			'-Werror=return-type',
			'-Werror=sequence-point',
			'-Werror=sizeof-pointer-memaccess',
			'-Werror=sizeof-array-div',
			'-Werror=sizeof-pointer-div',
			# '-Werror=strict-aliasing',
			'-Werror=string-compare',
			'-Werror=tautological-compare',
			'-Werror=use-after-free=3',
			'-Werror=vla',
			'-Werror=write-strings',

			# unstable diagnostics, may cause false positives
			'-Winit-self',
			'-Wmisleading-indentation',
			'-Wunintialized',

			# disabled, flood
			# '-Wdouble-promotion',
		]

		opt_cflags = [
			'-Werror=declaration-after-statement',
			'-Werror=enum-conversion',
			'-Werror=implicit-int',
			'-Werror=implicit-function-declaration',
			'-Werror=incompatible-pointer-types',
			'-Werror=int-conversion',
			'-Werror=jump-misses-init',
			# '-Werror=old-style-declaration',
			# '-Werror=old-style-definition',
			# '-Werror=strict-prototypes',
			'-fnonconst-initializers' # owcc
		]

		opt_cxxflags = [] # TODO:

		cflags = conf.filter_cflags(opt_flags + opt_cflags, cflags)
		cxxflags = conf.filter_cxxflags(opt_flags + opt_cxxflags, cxxflags)

		conf.env.append_unique('CFLAGS', cflags)
		conf.env.append_unique('CXXFLAGS', cxxflags)

	if conf.env.DEST_OS == 'android':
		# LIB_M added in xcompile!
		pass
	elif conf.env.DEST_OS == 'win32':
		a = [ 'user32', 'winmm' ]
		if conf.env.COMPILER_CC == 'msvc':
			for i in a:
				conf.start_msg('Checking for MSVC library')
				conf.check_lib_msvc(i)
				conf.end_msg(i)
		else:
			for i in a:
				conf.check_cc(lib = i)
	else:
		if conf.env.GOLDSOURCE_SUPPORT:
			conf.check_cc(lib='dl')
		conf.check_cc(lib='m')

	# check if we can use C99 tgmath
	if conf.check_cc(header_name='tgmath.h', mandatory=False):
		if conf.env.COMPILER_CC == 'msvc':
			conf.define('_CRT_SILENCE_NONCONFORMING_TGMATH_H', 1)
		tgmath_usable = conf.check_cc(fragment='''#include<tgmath.h>
			const float val = 2, val2 = 3;
			int main(void){ return (int)(-asin(val) + cos(val2)); }''',
			msg='Checking if tgmath.h is usable', mandatory=False, use='M')
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
		conf.env.append_unique('CXXFLAGS', ['-Wno-invalid-offsetof', '-fno-exceptions'])
		conf.define('stricmp', 'strcasecmp', quote=False)
		conf.define('strnicmp', 'strncasecmp', quote=False)
		conf.define('_snprintf', 'snprintf', quote=False)
		conf.define('_vsnprintf', 'vsnprintf', quote=False)
		conf.define('_LINUX', True)
		conf.define('LINUX', True)

	conf.msg(msg='-> processing mod options', result='...', color='BLUE')
	regex = re.compile('^([A-Za-z0-9_]+)=([A-Za-z0-9_]+)\ \#\ (.*)$')
	with open('mod_options.txt') as fd:
		lines = fd.readlines()
	for line in lines:
		m = regex.match(line.strip())
		if m:
			p = m.groups()
			conf.start_msg("* " + p[2])
			if p[1] == 'ON':
				conf.env[p[0]] = True
				conf.define(p[0], 1)
			elif p[1] == 'OFF':
				conf.env[p[0]] = False
				conf.undefine(p[0])
			else:
				conf.env[p[0]] = p[1]
			conf.end_msg(p[1])
	if conf.env.HLDEMO_BUILD and conf.env.OEM_BUILD:
		conf.fatal('Don\'t mix Demo and OEM builds!')

	# strip lib from pattern
	if conf.env.DEST_OS not in ['android']:
		if conf.env.cxxshlib_PATTERN.startswith('lib'):
			conf.env.cxxshlib_PATTERN = conf.env.cxxshlib_PATTERN[3:]

	conf.load('library_naming')
	conf.add_subproject('dlls')
	conf.add_subproject('cl_dll')

def build(bld):
	if bld.is_install and not bld.options.destdir:
		bld.fatal('Set the install destination directory using --destdir option')

	# don't clean QtCreator files and reconfigure saved options
	bld.clean_files = bld.bldnode.ant_glob('**',
		excl='*.user configuration.py .lock* *conf_check_*/** config.log %s/*' % Build.CACHE_DIR,
		quiet=True, generator=True)

	bld.add_subproject('dlls')
	bld.add_subproject('cl_dll')

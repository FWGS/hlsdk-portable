#! /usr/bin/env python
# Copyright 2019 (C) a1batross

from waflib import Configure, Errors, Utils

# TODO: make generic
CHECK_SYMBOL_EXISTS_FRAGMENT = '''
#include "build.h"

int main(int argc, char** argv)
{
  (void)argv;
#ifndef %s
  return ((int*)(&%s))[argc];
#else
  (void)argc;
  return 0;
#endif
}
'''

DEFINES = [
'XASH_64BIT',
'XASH_AMD64',
'XASH_ANDROID',
'XASH_APPLE',
'XASH_ARM',
'XASH_ARM_HARDFP',
'XASH_ARM_SOFTFP',
'XASH_ARMv4',
'XASH_ARMv5',
'XASH_ARMv6',
'XASH_ARMv7',
'XASH_ARMv8',
'XASH_BIG_ENDIAN',
'XASH_BSD',
'XASH_E2K',
'XASH_EMSCRIPTEN',
'XASH_FREEBSD',
'XASH_IOS',
'XASH_JS',
'XASH_LINUX',
'XASH_LITTLE_ENDIAN',
'XASH_MINGW',
'XASH_MIPS',
'XASH_MOBILE_PLATFORM',
'XASH_MSVC',
'XASH_NETBSD',
'XASH_OPENBSD',
'XASH_HAIKU',
'XASH_WIN32',
'XASH_WIN64',
'XASH_X86',
'XASH_DOS4GW',
'XASH_POSIX'
]

def configure(conf):
	conf.env.stash()
	conf.start_msg('Determining library postfix')
	tests = map(lambda x: {
		'fragment': CHECK_SYMBOL_EXISTS_FRAGMENT % (x, x),
		'includes': [conf.path.find_node('public/').abspath()],
		'define_name': x }, DEFINES )

	conf.multicheck(*tests, msg = '', mandatory = False, quiet = True)

	# engine/common/build.c
	if conf.env.XASH_ANDROID:
		buildos = "android"
	elif conf.env.XASH_WIN32 or conf.env.XASH_LINUX or conf.env.XASH_APPLE:
		buildos = "" # no prefix for default OS
	elif conf.env.XASH_FREEBSD:
		buildos = "freebsd"
	elif conf.env.XASH_NETBSD:
		buildos = "netbsd"
	elif conf.env.XASH_OPENBSD:
		buildos = "openbsd"
	elif conf.env.XASH_EMSCRIPTEN:
		buildos = "emscripten"
	elif conf.env.XASH_DOS4GW:
		buildos = "dos4gw" # unused, just in case
	elif conf.env.XASH_HAIKU:
		buildos = "haiku"
	else:
		conf.fatal("Place your operating system name in build.h and library_naming.py!\n"
			"If this is a mistake, try to fix conditions above and report a bug")

	if conf.env.XASH_AMD64:
		buildarch = "amd64"
	elif conf.env.XASH_X86:
		buildarch = ""
	elif conf.env.XASH_ARM and conf.env.XASH_64BIT:
		buildarch = "arm64"
	elif conf.env.XASH_ARM:
		buildarch = "armv"
		if conf.env.XASH_ARMv8:
			buildarch += "8_32"
		elif conf.env.XASH_ARMv7:
			buildarch += "7"
		elif conf.env.XASH_ARMv6:
			buildarch += "6"
		elif conf.env.XASH_ARMv5:
			buildarch += "5"
		elif conf.env.XASH_ARMv4:
			buildarch += "4"
		else:
			raise conf.fatal('Unknown ARM')
		
		if conf.env.XASH_ARM_HARDFP:
			buildarch += "hf"
		else:
			buildarch += "l"
	elif conf.env.XASH_MIPS and conf.env.XASH_BIG_ENDIAN:
		buildarch = "mips"
	elif conf.env.XASH_MIPS and conf.env.XASH_LITTLE_ENDIAN:
		buildarch = "mipsel"
	elif conf.env.XASH_JS:
		buildarch = "javascript"
	elif conf.env.XASH_E2K:
		buildarch = "e2k"
	else:
		raise conf.fatal("Place your architecture name in build.h and library_naming.py!\n"
			"If this is a mistake, try to fix conditions above and report a bug")
	
	conf.env.revert()
	
	if buildos == 'android':
		# force disable for Android, as Android ports aren't distributed in normal way and doesn't follow library naming
		conf.env.POSTFIX = ''
	elif buildos != '' and buildarch != '':
		conf.env.POSTFIX = '_%s_%s' % (buildos,buildarch)
	elif buildarch != '':
		conf.env.POSTFIX = '_%s' % buildarch
	else:
		conf.env.POSTFIX = ''
	
	conf.end_msg(conf.env.POSTFIX)

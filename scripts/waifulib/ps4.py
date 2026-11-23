# encoding: utf-8
# ps4.py -- PlayStation 4 (OpenOrbis toolchain) linking support
# SPDX-License-Identifier: Unlicense
# Written by Xash3D FWGS contributors, 2026, released into the public domain

# Every PS4 image is a PIE ELF linked against static musl libc of the toolchain and
# stubs of system libraries, then converted to a signed module (.prx) by create-fself.
# Game libraries are loaded by the engine, which owns the process heap: modules
# forward allocations there, see external/ps4compat/ps4_malloc.c.
#
# Runtime libraries must appear exactly once per image, but task generators
# often have both C and C++ link features, so we can't use LDFLAGS_<feature>
# variables and add them here instead.

import os
from waflib import Context, Task, TaskGen

PS4_SYSTEM_LIBS = ['-lSceNet', '-lkernel']

# file functions redirected to working directory emulation, see external/ps4compat/ps4_cwd.c
PS4_WRAPPED_FUNCS = ['open', 'fopen', 'stat', 'lstat', 'fstat', 'opendir', 'readdir', 'mkdir', 'rename', 'remove',
	'unlink', 'rmdir', 'access', 'chdir', 'getcwd', 'realpath', 'fcntl',
	# memory allocation goes to the process heap in eboot.bin, see ps4_malloc.c
	'malloc', 'free', 'calloc', 'realloc', 'memalign', 'aligned_alloc', 'valloc', 'posix_memalign',
	'malloc_usable_size',
	# standard output is made thread safe, see ps4_stdio.c
	'printf', 'vprintf', 'fprintf', 'vfprintf', 'puts', 'fputs', 'putchar', 'fputc', 'putc', 'fwrite']

PS4_PAID = '0x3800000000000011'

def configure(conf):
	# configuration tests are linked as programs with the startup object of the toolchain
	conf.env.PS4_CRT_PROGRAM = os.path.join(conf.env.PS4_TOOLCHAIN, 'lib', 'crt1.o')

	# replace -shared, modules are PIE images too
	for i in ['cprogram', 'cxxprogram', 'cshlib', 'cxxshlib']:
		conf.env['LINKFLAGS_' + i] = ['-Wl,-pie']

	conf.env.PS4_WRAP_FLAGS = ['-Wl,--wrap=%s' % i for i in PS4_WRAPPED_FUNCS]

	conf.find_program('create-fself', var = 'PS4_CREATE_FSELF', path_list = [os.path.join(conf.env.PS4_TOOLCHAIN, 'bin', 'linux')])

# runtime objects of external/ps4compat:
# ps4_cwd.c - working directory emulation
# ps4_crtlib.c - module startup code, replaces broken crtlib.o from the toolchain
# ps4_malloc.c - forwards allocations to the heap of the engine
# ps4_stdio.c - thread safe standard output
@TaskGen.feature('cshlib', 'cxxshlib')
@TaskGen.before_method('process_use')
def ps4_use_runtime(self):
	# configuration tests link without it, it's built later
	if self.env.DEST_OS != 'ps4' or getattr(self.bld, 'conf', None):
		return

	use = ['ps4compat_cwd', 'ps4compat_stdio', 'ps4compat_crtlib', 'ps4compat_malloc_module']
	self.use = self.to_list(getattr(self, 'use', [])) + use

@TaskGen.feature('cprogram', 'cxxprogram', 'cshlib', 'cxxshlib')
@TaskGen.after_method('propagate_uselib_vars')
def ps4_add_runtime(self):
	if self.env.DEST_OS != 'ps4' or getattr(self, 'ps4_runtime_added', False):
		return

	self.ps4_runtime_added = True

	is_library = 'cshlib' in self.features or 'cxxshlib' in self.features

	flags = [] if is_library else [self.env.PS4_CRT_PROGRAM, '-lSceSystemService']
	if not getattr(self.bld, 'conf', None):
		flags += self.env.PS4_WRAP_FLAGS
	if 'cxx' in self.features or 'cxxprogram' in self.features or 'cxxshlib' in self.features:
		flags += ['-lc++']
	flags += ['-lc'] + PS4_SYSTEM_LIBS

	self.env.append_value('LDFLAGS', flags)

class ps4_fself(Task.Task):
	"""Converts a game library to a signed module (.prx), writes names of its functions
	to .prx.sym: the engine needs them to save and load games, the console loader has none"""
	color = 'CYAN'

	def keyword(self):
		return 'Signing'

	def run(self):
		so = self.inputs[0].abspath()
		prx, oelf, sym = [i.abspath() for i in self.outputs]

		ret = self.exec_command(self.env.PS4_CREATE_FSELF + ['-in=%s' % so, '-out=%s' % oelf, '-lib=%s' % prx, '-paid', PS4_PAID],
			env = dict(os.environ, OO_PS4_TOOLCHAIN = self.env.PS4_TOOLCHAIN))
		if ret:
			return ret

		out = self.generator.bld.cmd_and_log(self.env.PS4_NM + ['--defined-only', '-n', so], quiet = Context.STDOUT)
		with open(sym, 'w') as f:
			for line in out.splitlines():
				parts = line.split()
				if len(parts) == 3 and parts[1] in 'TtWw':
					f.write('%s %s\n' % (parts[0], parts[2]))
		return 0

@TaskGen.feature('cshlib', 'cxxshlib')
@TaskGen.after_method('apply_link')
def ps4_make_prx(self):
	if self.env.DEST_OS != 'ps4' or getattr(self.bld, 'conf', None) or not getattr(self, 'link_task', None):
		return

	so = self.link_task.outputs[0]
	prx = so.change_ext('.prx')
	sym = so.change_ext('.prx.sym')
	self.create_task('ps4_fself', so, [prx, so.change_ext('.oelf'), sym])

	# the console needs only the module and its function names
	install_task = getattr(self, 'install_task', None)
	if install_task:
		install_task.hasrun = Task.SKIPPED
		install_task.no_errcheck_out = True
		self.add_install_files(install_to = install_task.install_to, install_from = [prx, sym])

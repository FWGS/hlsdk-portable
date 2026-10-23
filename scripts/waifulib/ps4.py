# encoding: utf-8
# ps4.py -- PlayStation 4 (OpenOrbis toolchain) linking support
# SPDX-License-Identifier: MIT
# Copyright (c) 2026 Xash3D FWGS contributors, see external/ps4compat/LICENSE

# Every PS4 image (eboot.bin or .prx module) is a PIE ELF linked against
# OpenOrbis startup object, static musl libc and stubs of system libraries,
# later converted to signed ELF by create-fself (see ps4_make_prx below).
#
# Startup object and runtime libraries must appear exactly once per image,
# but task generators often have both C and C++ link features, so we
# can't use LDFLAGS_<feature> variables and add them here instead.

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

PS4_MALLOC_FUNCS = ['malloc', 'free', 'calloc', 'realloc', 'memalign', 'aligned_alloc', 'valloc',
	'posix_memalign', 'malloc_usable_size']

def configure(conf):
	toolchain = conf.env.PS4_TOOLCHAIN
	conf.env.PS4_CRT_PROGRAM = os.path.join(toolchain, 'lib', 'crt1.o')

	# replace -shared, modules are PIE images too
	for i in ['cprogram', 'cxxprogram', 'cshlib', 'cxxshlib']:
		conf.env['LINKFLAGS_' + i] = ['-Wl,-pie']

	# runtime objects linked into every image:
	# ps4_cwd.c - working directory emulation
	# ps4_crtlib.c - module startup code, replaces broken crtlib.o from the toolchain
	# ps4_malloc.c - process-wide heap
	# ps4_stdio.c - thread safe standard output
	# (object name, source, extra flags)
	objects = [
		('cwd', 'ps4_cwd', []),
		('stdio', 'ps4_stdio', []),
		('crtlib', 'ps4_crtlib', []),
		('malloc', 'ps4_malloc', []),
		('malloc_module', 'ps4_malloc', ['-DPS4_MODULE=1']),
	]

	for key, name, extra in objects:
		for i in ['external/ps4compat/']:
			src = conf.path.find_node(i + name + '.c')
			if src:
				break
		else:
			conf.fatal('%s.c not found' % name)

		obj = conf.bldnode.make_node('ps4_rt_%s.o' % key)
		conf.start_msg('Compiling PS4 runtime %s' % key)
		cmd = conf.env.CC + conf.env.CFLAGS + extra + ['-O2', '-c', src.abspath(), '-o', obj.abspath()]
		try:
			conf.cmd_and_log(cmd)
		except Exception as e:
			conf.end_msg('failed', color='RED')
			conf.fatal('Failed to compile %s: %s' % (src.abspath(), e))
		conf.end_msg('ok')

		conf.env['PS4_RT_' + key.upper()] = obj.abspath()

	# diagnostic switch: PS4_NO_MALLOC_WRAP=1 keeps original per-image musl malloc
	conf.env.PS4_MALLOC_WRAP = not os.environ.get('PS4_NO_MALLOC_WRAP')
	funcs = PS4_WRAPPED_FUNCS
	if not conf.env.PS4_MALLOC_WRAP:
		funcs = [i for i in funcs if i not in PS4_MALLOC_FUNCS]
	conf.msg('PS4 shared locked heap', conf.env.PS4_MALLOC_WRAP)
	conf.env.PS4_WRAP_FLAGS = ['-Wl,--wrap=%s' % i for i in funcs]

	# libraries of the game are loaded as modules, they are converted to signed ELF
	conf.find_program('create-fself', path_list=[os.path.join(toolchain, 'bin', 'linux')], var='PS4_CREATE_FSELF')
	conf.find_program(['llvm-nm-18', 'llvm-nm'], var='PS4_NM')

@TaskGen.feature('cprogram', 'cxxprogram', 'cshlib', 'cxxshlib')
@TaskGen.after_method('propagate_uselib_vars')
def ps4_add_runtime(self):
	if self.env.DEST_OS != 'ps4' or getattr(self, 'ps4_runtime_added', False):
		return

	self.ps4_runtime_added = True

	is_library = 'cshlib' in self.features or 'cxxshlib' in self.features
	is_cxx = 'cxx' in self.features or 'cxxprogram' in self.features or 'cxxshlib' in self.features

	if is_library:
		flags = [self.env.PS4_RT_CRTLIB]
		if self.env.PS4_MALLOC_WRAP:
			flags += [self.env.PS4_RT_MALLOC_MODULE]
	else:
		flags = [self.env.PS4_CRT_PROGRAM, '-lSceSystemService']
		if self.env.PS4_MALLOC_WRAP:
			# eboot.bin owns the process heap, modules find it at runtime
			flags += [self.env.PS4_RT_MALLOC]
	flags += [self.env.PS4_RT_CWD, self.env.PS4_RT_STDIO]
	flags += self.env.PS4_WRAP_FLAGS
	if is_cxx:
		flags += ['-lc++']
	flags += ['-lc'] + PS4_SYSTEM_LIBS

	self.env.append_value('LDFLAGS', flags)

@TaskGen.feature('cprogram', 'cxxprogram', 'cshlib', 'cxxshlib')
@TaskGen.after_method('apply_link')
def ps4_runtime_deps(self):
	# runtime objects are passed through LDFLAGS, so waf doesn't know
	# images have to be relinked when they change
	if self.env.DEST_OS != 'ps4' or not getattr(self, 'link_task', None):
		return

	for key in ['PS4_RT_CWD', 'PS4_RT_STDIO', 'PS4_RT_CRTLIB', 'PS4_RT_MALLOC', 'PS4_RT_MALLOC_MODULE']:
		path = self.env[key]
		if path:
			node = self.bld.root.find_node(path)
			if node:
				self.link_task.dep_nodes.append(node)

class ps4_fself(Task.Task):
	"""Converts a game library to a signed module (.prx), writes names of its functions
	to .prx.sym: the engine needs them to save and load games, the console loader has none"""
	color = 'CYAN'

	def run(self):
		so = self.inputs[0].abspath()
		prx = self.outputs[0].abspath()
		sym = self.outputs[1].abspath()

		ret = self.exec_command(self.env.PS4_CREATE_FSELF + ['-in=' + so, '-out=' + prx + '.oelf', '--lib=' + prx,
			'--paid', '0x3800000000000011'])
		if os.path.exists(prx + '.oelf'):
			os.remove(prx + '.oelf')
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
	if self.env.DEST_OS != 'ps4' or not getattr(self, 'link_task', None):
		return

	so = self.link_task.outputs[0]
	prx = so.change_ext('.prx')
	sym = so.change_ext('.prx.sym')
	self.create_task('ps4_fself', so, [prx, sym])

	if getattr(self, 'install_path', None):
		self.add_install_files(install_to = self.install_path, install_from = [prx, sym])

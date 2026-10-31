# PlayStation 4 game libraries, see cmake/PS4Toolchain.cmake
#
# Every library is a PIE image linked with external/ps4compat, which fixes the runtime
# of OpenOrbis (working directory, process heap of the engine, thread safe stdout),
# then it's converted to a signed module (.prx) by create-fself. The engine also
# needs names of functions in .prx.sym to save and load games, the console loader has none.

include_guard(GLOBAL)

set(PS4_CMAKE_DIR "${CMAKE_CURRENT_LIST_DIR}")

# replace -shared, modules are PIE images too
set(CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS "-Wl,-pie")
set(CMAKE_SHARED_LIBRARY_CREATE_CXX_FLAGS "-Wl,-pie")

find_program(PS4_CREATE_FSELF create-fself PATHS "${OO_PS4_TOOLCHAIN}/bin/linux" NO_DEFAULT_PATH)
if(NOT PS4_CREATE_FSELF)
	message(FATAL_ERROR "create-fself not found in ${OO_PS4_TOOLCHAIN}/bin/linux")
endif()

# llvm-nm of the same LLVM as the compiler
get_filename_component(PS4_LLVM_BIN "${CMAKE_C_COMPILER}" REALPATH)
get_filename_component(PS4_LLVM_BIN "${PS4_LLVM_BIN}" DIRECTORY)
find_program(PS4_NM NAMES llvm-nm PATHS "${PS4_LLVM_BIN}" NO_DEFAULT_PATH)
if(NOT PS4_NM)
	find_program(PS4_NM NAMES llvm-nm-18 llvm-nm)
endif()
if(NOT PS4_NM)
	message(FATAL_ERROR "llvm-nm not found")
endif()

set(PS4COMPAT_DIR "${PS4_CMAKE_DIR}/../external/ps4compat")
add_library(ps4compat OBJECT
	"${PS4COMPAT_DIR}/ps4_cwd.c"
	"${PS4COMPAT_DIR}/ps4_stdio.c"
	"${PS4COMPAT_DIR}/ps4_crtlib.c"
	"${PS4COMPAT_DIR}/ps4_malloc.c")
# modules forward allocations to the heap of the engine
set_source_files_properties("${PS4COMPAT_DIR}/ps4_malloc.c" PROPERTIES COMPILE_DEFINITIONS PS4_MODULE=1)

# file functions go to working directory emulation, memory to the heap of the engine,
# standard output is made thread safe
set(PS4_WRAPPED_FUNCS open fopen stat lstat fstat opendir readdir mkdir rename remove
	unlink rmdir access chdir getcwd realpath fcntl
	malloc free calloc realloc memalign aligned_alloc valloc posix_memalign malloc_usable_size
	printf vprintf fprintf vfprintf puts fputs putchar fputc putc fwrite)

function(ps4_add_module target destination)
	target_sources(${target} PRIVATE $<TARGET_OBJECTS:ps4compat>)
	foreach(func ${PS4_WRAPPED_FUNCS})
		target_link_libraries(${target} "-Wl,--wrap=${func}")
	endforeach()

	get_target_property(name ${target} OUTPUT_NAME)
	set(prx "${CMAKE_CURRENT_BINARY_DIR}/${name}.prx")

	add_custom_command(TARGET ${target} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E env "OO_PS4_TOOLCHAIN=${OO_PS4_TOOLCHAIN}"
			"${PS4_CREATE_FSELF}" "-in=$<TARGET_FILE:${target}>" "-out=${prx}.oelf" "-lib=${prx}" -paid 0x3800000000000011
		COMMAND ${CMAKE_COMMAND} "-DNM=${PS4_NM}" "-DINPUT=$<TARGET_FILE:${target}>" "-DOUTPUT=${prx}.sym"
			-P "${PS4_CMAKE_DIR}/PS4MakeSymbols.cmake"
		VERBATIM)

	install(FILES "${prx}" "${prx}.sym" DESTINATION "${destination}")
endfunction()

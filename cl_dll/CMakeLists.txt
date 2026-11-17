cmake_minimum_required(VERSION 3.9)
project(CLDLL)

set(CLDLL_LIBRARY client)

file(GLOB CZ_CLIENT_SRC "*.cpp")
file(GLOB CZ_EVENTS_SRC "events/*.cpp")
file(GLOB CZ_WPN_SRC "cs_wpn/*.cpp")
file(GLOB CZ_WPNSH_SRC "cs16_dlls/wpn_shared/*.cpp")
file(GLOB CZ_HUD_SRC "hud/*.cpp")
file(GLOB CZ_INPUT_SRC "input/*.cpp")
file(GLOB CZ_PARTICLEMAN_SRC "particleman/*.cpp")
file(GLOB CZ_MINIUTL_SRC "../external/miniutl/*.cpp")

list(REMOVE_ITEM CZ_CLIENT_SRC "${CMAKE_CURRENT_SOURCE_DIR}/ev_hldm.cpp")
list(REMOVE_ITEM CZ_CLIENT_SRC "${CMAKE_CURRENT_SOURCE_DIR}/inputw32.cpp")
list(REMOVE_ITEM CZ_INPUT_SRC "${CMAKE_CURRENT_SOURCE_DIR}/input/input_sdl.cpp")
list(REMOVE_ITEM CZ_WPN_SRC "${CMAKE_CURRENT_SOURCE_DIR}/cs_wpn/cs_baseentity.cpp")

set(CZ_SOURCES
	${CZ_CLIENT_SRC}
	${CZ_EVENTS_SRC}
	${CZ_WPN_SRC}
	${CZ_WPNSH_SRC}
	${CZ_HUD_SRC}
	${CZ_INPUT_SRC}
	${CZ_PARTICLEMAN_SRC}
	${CZ_MINIUTL_SRC}
	../common/interface.cpp
	../public/utflib.cpp
	../pm_shared/pm_shared.c
	../pm_shared/pm_math.c
	../pm_shared/pm_debug.c)

# the shared movement code is C, the client includes its headers as C++
set_source_files_properties(
	../pm_shared/pm_shared.c
	../pm_shared/pm_math.c
	../pm_shared/pm_debug.c
	PROPERTIES LANGUAGE CXX)

if(HAVE_STRLCPY)
	add_definitions(-DHAVE_STRLCPY)
else()
	list(APPEND CZ_SOURCES ../public/strlcpy.c)
endif()

if(HAVE_STRLCAT)
	add_definitions(-DHAVE_STRLCAT)
else()
	list(APPEND CZ_SOURCES ../public/strlcat.c)
endif()

include_directories(
	include
	include/hud
	include/studio
	include/math
	.
	../common
	../engine
	../pm_shared
	cs16_dlls
	../game_shared
	../public
	../public/cl_dll
	../external/miniutl
	particleman)

add_definitions(-DCLIENT_WEAPONS -DCLIENT_DLL -DSTDINT_H=<cstdint>)

if(NOT MSVC)
	add_definitions(-DLINUX -D_LINUX)
	add_definitions(-Dstricmp=strcasecmp -D_strnicmp=strncasecmp -Dstrnicmp=strncasecmp -fms-extensions)
else()
	add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_DEPRECATE)
endif()

add_library(${CLDLL_LIBRARY} SHARED ${CZ_SOURCES})

set_target_postfix(${CLDLL_LIBRARY})

if(HAVE_LTO)
	set_property(TARGET ${CLDLL_LIBRARY} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

install(TARGETS ${CLDLL_LIBRARY}
	RUNTIME DESTINATION "${GAMEDIR}/${CLIENT_INSTALL_DIR}/"
	LIBRARY DESTINATION "${GAMEDIR}/${CLIENT_INSTALL_DIR}/"
	PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
		GROUP_READ GROUP_EXECUTE
		WORLD_READ WORLD_EXECUTE)

if(MSVC)
	install(FILES $<TARGET_PDB_FILE:${CLDLL_LIBRARY}> DESTINATION "${GAMEDIR}/${CLIENT_INSTALL_DIR}/" OPTIONAL)
endif()

if(CMAKE_BUILD_TYPE MATCHES "Release" AND NOT WIN32)
	if(APPLE)
		set(STRIP_FLAGS -x)
	else()
		set(STRIP_FLAGS -s)
	endif()
	add_custom_command(TARGET ${CLDLL_LIBRARY}
			POST_BUILD DEPENDS ${CLDLL_LIBRARY}
			COMMAND ${CMAKE_STRIP} ${STRIP_FLAGS} $<TARGET_FILE:${CLDLL_LIBRARY}>)
endif()

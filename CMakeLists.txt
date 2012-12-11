project(Portage-Config-Cleaner)

cmake_minimum_required( VERSION 2.8.8 )

set(PCC_VERSION_MAJOR 0)
set(PCC_VERSION_MINOR 1)
set(PCC_VERSION_PATCH 0)
set(PCC_VERSION "${PCC_VERSION_MAJOR}.${PCC_VERSION_MINOR}.${PCC_VERSION_PATCH}")

set(CPACK_PACKAGE_VERSION_MAJOR ${PCC_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PCC_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PCC_VERSION_PATCH})
#set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE.txt")
#set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README")
#set(CPACK_PACKAGE_VENDOR "Lua.org, PUC-Rio.")
set(CPACK_GENERATOR "TBZ2")
set(CPACK_SOURCE_GENERATOR "TBZ2")
set(CPACK_SOURCE_IGNORE_FILES
	".git;build")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${PCC_MODULE_VERSION}")
# MUST be after CPACK_* variables.
include(CPack)

#include(CheckIncludeFiles)
#include(CheckCSourceCompiles)

#set(BZIP-VERSION 1.0.6)
#set(FREETYPE-VERSION 2.4.10)
#set(LUA-VERSION 5.2.1)

# hack to use my lua-5.2 search module
#SET( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake_extra )

# installation directory configuration
set(INST_PREFIX "/usr" CACHE PATH "Prefix prepended to install directories")
set(CMAKE_INSTALL_PREFIX "${INST_PREFIX}" CACHE INTERNAL "Prefix prepended to install directories" FORCE)

# some hints about application build configuration
#message(STATUS "config variables: build-debug enable-tests extra-utils")
#message(STATUS "config variables: to enable or disable them use -Dvar-name=1 or 0, -U is equivalent for -D...=0")
#if (UNIX)
#	message(STATUS "config variables: for example: -Dbuild-debug=1")
#else (UNIX)
#	message(STATUS "config variables: for example: -Dbuild-debug=1")
#endif (UNIX)

#message(STATUS "warning: application performance with enabled lua hooks can be much lower depending on hook scripts")
#message(STATUS "warning: shared memory (shmem) feature isn't completely tested")

message(STATUS "To change installation directory use -DINST_PREFIX=path, for example, -DINST_PREFIX=/usr")

# don't USE -O3 with GCC, it causes less precise calculations
if (CMAKE_COMPILER_IS_GNUCC)
	set (CMAKE_C_FLAGS_RELEASE "-O2 -pipe -march=native -Wall -Wextra")
	set (CMAKE_CXX_FLAGS_RELEASE ${CMAKE_C_FLAGS_RELEASE})
endif (CMAKE_COMPILER_IS_GNUCC)

# clang compiler options
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	set (CLANG_WARNINGS "-Weverything -Wno-missing-prototypes -Wno-padded -Wno-sign-conversion -Wno-conversion -Wno-disabled-macro-expansion -Wno-cast-align" )
	set (CMAKE_C_FLAGS_RELEASE "-O3 -pipe ${CLANG_WARNINGS}")
	set (CMAKE_CXX_FLAGS_RELEASE ${CMAKE_C_FLAGS_RELEASE})
endif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")

add_definitions(-D_FILE_OFFSET_BITS=64)

set ( SOURCES main.cpp )
set ( HEADERS )

add_executable( portage-config-cleaner ${SOURCES} ${HEADERS})
target_link_libraries( portage-config-cleaner )

# installation config
install(TARGETS portage-config-cleaner RUNTIME DESTINATION ./bin)

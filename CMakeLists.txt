project(Portage-Config-Cleaner)

cmake_minimum_required( VERSION 2.8.8 )

set(PCC_VERSION_MAJOR 0)
set(PCC_VERSION_MINOR 1)
set(PCC_VERSION_PATCH 0)
set(PCC_VERSION "${PCC_VERSION_MAJOR}.${PCC_VERSION_MINOR}.${PCC_VERSION_PATCH}")

set(CPACK_PACKAGE_VERSION_MAJOR ${PCC_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PCC_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PCC_VERSION_PATCH})
set(CPACK_GENERATOR "TBZ2")
set(CPACK_SOURCE_GENERATOR "TBZ2")
set(CPACK_SOURCE_IGNORE_FILES
	".git;build;CMakeLists.txt.user")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${PCC_VERSION}")
# MUST be after CPACK_* variables.
include(CPack)

# installation directory configuration
set(INST_PREFIX "/usr" CACHE PATH "Prefix prepended to install directories")
set(CMAKE_INSTALL_PREFIX "${INST_PREFIX}" CACHE INTERNAL "Prefix prepended to install directories" FORCE)

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

set ( SOURCES atom.cpp use_flag.cpp read_file.cpp main.cpp )
set ( HEADERS atom.h   use_flag.h   read_file.h )

add_executable( portage-config-cleaner ${SOURCES} ${HEADERS})
target_link_libraries( portage-config-cleaner )

# installation config
install(TARGETS portage-config-cleaner RUNTIME DESTINATION ./bin)

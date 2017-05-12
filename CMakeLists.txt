project(Portage-Config-Checker)

cmake_minimum_required( VERSION 2.8.8 )

set(PCC_VERSION_MAJOR 0)
set(PCC_VERSION_MINOR 5)
set(PCC_VERSION_PATCH 1)
set(PCC_VERSION "${PCC_VERSION_MAJOR}.${PCC_VERSION_MINOR}.${PCC_VERSION_PATCH}")

set(CPACK_PACKAGE_VERSION_MAJOR ${PCC_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PCC_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PCC_VERSION_PATCH})
set(CPACK_GENERATOR "TBZ2")
set(CPACK_SOURCE_GENERATOR "TBZ2")
set(CPACK_SOURCE_IGNORE_FILES
	".git;build;CMakeLists.txt.user;CMakeLists.txt.user.*;TODO")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${PCC_VERSION}")
# MUST be after CPACK_* variables.
include(CPack)

# installation directory configuration
set(LIB_SUFFIX "" CACHE STRING "Define suffix of directory name (32/64)")
set(EXEC_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX} CACHE PATH "Installation prefix for executables and object code libraries" FORCE)
set(BIN_INSTALL_DIR ${EXEC_INSTALL_PREFIX}/bin CACHE PATH "Installation prefix for user executables" FORCE)
set(SBIN_INSTALL_DIR ${EXEC_INSTALL_PREFIX}/sbin CACHE PATH "Installation prefix for user executables" FORCE)
set(LIB_INSTALL_DIR ${EXEC_INSTALL_PREFIX}/lib${LIB_SUFFIX} CACHE PATH  "Installation prefix for object code libraries" FORCE)
set(INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/include CACHE PATH "Installation prefix for C header files" FORCE)
set(DATA_PREFIX "${CMAKE_INSTALL_PREFIX}/share" CACHE PATH "Prefix prepended to data install directories" FORCE)

FIND_PACKAGE( Boost COMPONENTS regex REQUIRED )
INCLUDE_DIRECTORIES( ${Boost_INCLUDE_DIR} )

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

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")

add_definitions(-D_FILE_OFFSET_BITS=64)

set ( SOURCES atom.cpp use_flag.cpp read_file.cpp main.cpp )
set ( HEADERS atom.h   use_flag.h   read_file.h )

add_executable( portage-config-checker ${SOURCES} ${HEADERS})
target_link_libraries( portage-config-checker ${Boost_LIBRARIES} )

# installation config
install(TARGETS portage-config-checker RUNTIME DESTINATION ${BIN_INSTALL_DIR})

cmake_minimum_required (VERSION 2.8)
project (loopAdapt)

set(loopAdapt_VERSION_MAJOR 0)
set(loopAdapt_VERSION_MINOR 0)
set(loopAdapt_VERSION_REVISION 0)
set(loopAdapt_VERSION_IDENTIFIER)
set(loopAdapt_VERSION "${loopAdapt_VERSION_MAJOR}.${loopAdapt_VERSION_MINOR}.${loopAdapt_VERSION_REVISION}")

include(ExternalProject)

set(loopAdapt_VERBOSITY 1 CACHE STRING "Level of verbosity information. Valid values: 0 (be totally silent), 1 (print warnings, errors and further important information) 2 (print all outputs)")

if (NOT BUILD_SHARED_LIBS)
    message(STATUS "No library type specified. Setting it to shared.")
    set(BUILD_SHARED_LIBS ON CACHE BOOL "Set to ON/OFF for shared/static library build.")
endif()


set (SOURCES
    src/ghash.c
    src/loop_adapt.c
    src/loop_adapt_eval.c
    src/loop_adapt_eval_blocksize.c
    src/loop_adapt_eval_dvfs.c
    src/loop_adapt_helper.c
    src/loop_adapt_calc.c
    )

set (HEADERS
    include/loop_adapt.h
    )

set(USE_LIKWID ON CACHE BOOL "Switch LIKWID ON or OFF")

if (${USE_LIKWID})
    set(LIKWID_INCLUDE_DIR "" CACHE PATH "Location of likwid.h")
    include_directories(${LIKWID_INCLUDE_DIR})
    get_filename_component(LIKWID_LIBDIR ${LIKWID_INCLUDE_DIR}/../lib REALPATH)
    find_library(LIKWID_LIBRARIES likwid ${LIKWID_LIBDIR})
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DLIKWID_PERFMON")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DLIKWID_PERFMON")
endif()

set(loopAdapt_USE_OpenMP 1 CACHE BOOL "Build with OpenMP")

set(HWLOC_INCLUDE_DIR HWLOC_INCLUDE_DIR-NOTFOUND CACHE PATH "Location of hwloc.h")
set(LIBHWLOC LIBHWLOC-NOTFOUND)

if (PKGCONFIG_FOUND AND NOT HWLOC_INCLUDE_DIR)
    message(STATUS "Try to find hwloc via pkg-config")
    find_package(HWLOC)
    find_library(LIBHWLOC hwloc)
endif()

# find hwloc.h in default header locations
if (NOT HWLOC_INCLUDE_DIR)
    find_path(HWLOC_INCLUDE_DIR hwloc.h)
endif()

#set(GITHUB_GIT_URL "github.com" CACHE STRING "The github GIT URL. This has to be changed in certain situations if located behind a firewall.")
set(BUILTIN_HWLOC false)

# hwloc not found - download and build it
if (NOT HWLOC_INCLUDE_DIR)
    message(STATUS "Could not find hwloc.h neither via pkg-config nor in the defined HWLOC_INCLUDE_DIR! Trying to build hwloc...")
    ExternalProject_Add(hwloc
        PREFIX ${PROJECT_BINARY_DIR}/hwloc
        URL https://www.open-mpi.org/software/hwloc/v1.11/downloads/hwloc-1.11.6.tar.gz
        SOURCE_DIR ${PROJECT_BINARY_DIR}/hwloc/src/hwloc
        #CONFIGURE_COMMAND ./configure --enable-static --disable-shared --prefix=${PROJECT_BINARY_DIR}/hwloc-inst
        CONFIGURE_COMMAND CXXFLAGS=-fPIC CFLAGS=-fPIC ./configure --prefix=${PROJECT_BINARY_DIR}/hwloc-inst
        BUILD_COMMAND make
        BUILD_IN_SOURCE 1
        INSTALL_COMMAND make install
        )
    set(BUILTIN_HWLOC 1)
    set(HWLOC_INCLUDE_DIR "${PROJECT_BINARY_DIR}/hwloc-inst/include")
    set(LIBHWLOC "${PROJECT_BINARY_DIR}/hwloc-inst/lib/libhwloc.so")
endif()

if (NOT LIBHWLOC)
    get_filename_component(HWLOC_LIBDIR ${HWLOC_INCLUDE_DIR}/../lib REALPATH)
    find_library(LIBHWLOC hwloc HINTS ${HWLOC_LIBDIR})
endif()
set(HWLOC_LIBRARIES ${LIBHWLOC})

if(loopAdapt_USE_OpenMP)
    find_package(OpenMP)
    if (OPENMP_FOUND)
        set(loopAdapt_HAVE_OpenMP 1)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    else()
        message(FATAL_ERROR "Couldn't find OpenMP")
        set(loopAdapt_HAVE_OpenMP 0)
    endif()
else ()
    message(STATUS "no check ${OpenMP_CXX_FLAGS}")
    set(loopAdapt_HAVE_OpenMP 0)
endif()

set(loopAdapt_USE_MPI 0 CACHE BOOL "Build with MPI")

if(loopAdapt_USE_MPI)
    set(MPI_LIBRARIES "-lmpi_mt")
    find_package(MPI)
    if (MPI_FOUND)
        include_directories(SYSTEM ${MPI_INCLUDE_PATH})
        foreach(item ${MPI_LIBRARIES})
            string(REGEX MATCH ".*libmpi.so" match ${item})
            if(match)
                get_filename_component(MPI_LIBDIR ${match} PATH)
                if (EXISTS "${MPI_LIBDIR}/release_mt/")
                    list(REMOVE_ITEM MPI_LIBRARIES ${item})
                    list(APPEND MPI_LIBRARIES
                        ${MPI_LIBDIR}/release_mt/libmpi.so)
                endif()
                if(EXISTS "${MPI_LIBDIR}/libmpi_mt.so")
                    list(REMOVE_ITEM MPI_LIBRARIES ${item})
                    list(APPEND MPI_LIBRARIES ${MPI_LIBDIR}/libmpi_mt.so)
                endif()
            endif()
        endforeach()
        get_filename_component(MPI_LIBDIR "${MPI_LIBRARY}" PATH)
        find_library(LIBMPI_CXX mpi_cxx ${MPI_LIBDIR})
        set(loopAdapt_HAVE_MPI 1)
        mark_as_advanced(MPI_EXTRA_LIBRARY MPI_LIBRARY LIBMPI_CXX)
        mark_as_advanced(CLEAR MPI_C_INCLUDE_PATH)
    endif()
endif()

set(loopAdapt_PRINT_RANK 0 CACHE STRING "MPI Print RANK")

#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_GNU_SOURCE -O3 -fPIC -std=gnu99 -Wall -Wextra -Werror -pedantic")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_GNU_SOURCE -O3 -fPIC -std=gnu99 -Wno-format")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_GNU_SOURCE -fPIC -std=c++0x -Wall -Wextra -Werror -pedantic")
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
    set(CMAKE_INCLUDE_SYSTEM_FLAG_C "-isystem ") # Not present in Cmake 2.8.11.2
    set(CMAKE_INCLUDE_SYSTEM_FLAG_CXX "-isystem ") # Not present in Cmake 2.8.11.2
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wunused-variable -save-temps -fsource-asm -w3 -wd981")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wunused-variable -save-temps -fsource-asm -w3 -wd981 -wd383")
    if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-alias")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-alias")
    endif()
else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unknown-pragmas")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unknown-pragmas")
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU") 
        message(STATUS "NOTE: by default, we only set the -save-temps compiler flag with Intel compilers right now. You may want to use CC=\"ccache gcc\" etc to speed up the compiliation69.")
        if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Ofast -fstrict-aliasing")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Ofast -fstrict-aliasing")
        endif()
        if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" AND NOT "${CMAKE_CXX_COMPILER_VERSION}" VERSION_LESS "4.8"))
            set(SANITIZER "" CACHE STRING "What should be sanitized by Clang/GCC for a build? (address, thread, bounds,..., undefined or leave empty), use semicolon separated list for multiple")
            if (NOT "${SANITIZER}" STREQUAL "")
                set(SANITIZE_STRING "")
                FOREACH(sanitize_obj ${SANITIZER})
                    string(TOLOWER "${sanitize_obj}" sanitize_obj)
                    message(STATUS "check ${sanitize_obj}")
                    set(SANITIZE_STRING "${SANITIZE_STRING} -fsanitize=${sanitize_obj}")
                ENDFOREACH(sanitize_obj)
                set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g ${SANITIZE_STRING} -fno-omit-frame-pointer")
                set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g ${SANITIZE_STRING} -fno-omit-frame-pointer")
            endif()
        endif()
    endif()
endif()

configure_file (
    "${PROJECT_SOURCE_DIR}/include/config.h.in"
    "${PROJECT_SOURCE_DIR}/include/config.h"
    )

configure_file(${PROJECT_SOURCE_DIR}/loopAdaptConfig.cmake.in ${PROJECT_BINARY_DIR}/loopAdaptConfig.cmake @ONLY)

include_directories(${PROJECT_SOURCE_DIR}/include
                   ${PROJECT_SOURCE_DIR}/include/lua)
add_library(loopAdapt ${SOURCES})

include_directories(SYSTEM "${HWLOC_INCLUDE_DIR}")

if(BUILTIN_HWLOC)
    target_link_libraries(loopAdapt "${LIBHWLOC}")
    add_dependencies(loopAdapt hwloc)
endif()

install(FILES ${HEADERS} DESTINATION include/loopAdapt/)

if (${BUILD_SHARED_LIBS})
    install(TARGETS loopAdapt EXPORT loopAdapt-targets LIBRARY DESTINATION lib/loopAdapt)
else ()
    install(TARGETS loopAdapt EXPORT loopAdapt-targets ARCHIVE DESTINATION lib/loopAdapt)
endif()

install(FILES ${PROJECT_BINARY_DIR}/loopAdaptConfig.cmake DESTINATION lib/loopAdapt)

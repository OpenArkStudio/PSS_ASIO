cmake_minimum_required(VERSION 3.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

project(PSS_alpha)

if(UNIX)
	#linux
	include(CheckCXXCompilerFlag)
	CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
	if(COMPILER_SUPPORTS_CXX11)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
	endif()
	
	#message("[pss build]build jemalloc begin")
	#execute_process(COMMAND sh jemalloc.sh WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
	#message("[pss build]build jemalloc end")
	
	message("[pss build]check jemalloc")
	unset(libjemalloc CACHE) 
	find_library(libjemalloc NAMES jemalloc PATHS ${PROJECT_SOURCE_DIR}/Build/Linux/commonlib)
	if (${libjemalloc} STREQUAL libjemalloc-NOTFOUND)
		message("[pss build]build jemalloc begin")
		execute_process(COMMAND sh jemalloc.sh WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
		message("[pss build]build jemalloc end")
	endif()	
	
	unset(libjemalloc CACHE) 
	find_library(libjemalloc NAMES jemalloc PATHS ${PROJECT_SOURCE_DIR}/Build/Linux/commonlib)
	#message("[pss build]libjemalloc= ${libjemalloc}")
	if (${libjemalloc} STREQUAL libjemalloc-NOTFOUND)
		message("[pss build]jemallocis install fail")
	endif()
	
endif()

set(ROOT_DIR ${PROJECT_SOURCE_DIR})

execute_process(
    COMMAND git describe --abbrev=8 --always --tags
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

message("GIT_HASH: ${GIT_HASH}")

execute_process(
    COMMAND git symbolic-ref --short -q HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_BRANCH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

message("GIT_BRANCH: ${GIT_BRANCH}")
string(TIMESTAMP COMPILE_TIME %Y-%m-%d_%H:%M:%S)
set(build_time ${COMPILE_TIME})

message("build_time:${build_time}")

configure_file(${ROOT_DIR}/Common/VersionConfig.h.in ${ROOT_DIR}/Common/VersionConfig.h)

if(UNIX)
	#linux
	if(CMAKE_BUILD_TYPE MATCHES "Debug")
		message("[PSS_ASIO]set Debug...")
		set(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -fPIC")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -D_DEBUG")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g -ggdb -fPIC")
	else()
		message("[PSS_ASIO]set Release...")
		set(CMAKE_BUILD_TYPE "Release")
		set(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -fPIC")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -DNDEBUG")
	endif()
	set(LDFLAGS "$ENV{LDFLAGS} -stdlib=libc++")
	#add_definitions(-Wno-unused-variable -Wno-unused-parameter -Wno-unused-function -Wno-strict-aliasing -Wno-pmf-conversions)
	
	set(BIN_OUTPUT_DIR ${ROOT_DIR}/Build/Linux)
else()
	#windows
	if(CMAKE_BUILD_TYPE MATCHES "debug")
		add_definitions(-D_DEBUG)
	else(CMAKE_BUILD_TYPE MATCHES "debug")
		add_definitions(-DNDEBUG)
	endif(CMAKE_BUILD_TYPE MATCHES "debug")
endif()

#add your own sub dirs
add_subdirectory(PSS_ASIO)
add_subdirectory(Module_Logic/Test_Logic)
add_subdirectory(PacketParse_Interface)
add_subdirectory(PSS_ASIO_CLIENT)
#TODO:add other dirs

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
endif()

set(ROOT_DIR ${PROJECT_SOURCE_DIR})

if(UNIX)
	#linux
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g -ggdb -fPIC")
	if(CMAKE_BUILD_TYPE MATCHES "debug")
		set(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -fPIC")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -D_DEBUG")
	else()
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
add_subdirectory(PacketParse_Inferface)
#TODO:add other dirs

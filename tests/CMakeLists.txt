project(test_pss_asio)
cmake_minimum_required(VERSION 3.0)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

if(UNIX)
	set(BIN_OUTPUT_DIR ${PROJECT_SOURCE_DIR})
	set(LIB_OUTPUT_DIR ${PROJECT_SOURCE_DIR})
else()
	set(BIN_OUTPUT_DIR ${PROJECT_SOURCE_DIR})
	set(LIB_OUTPUT_DIR ${PROJECT_SOURCE_DIR})
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${BIN_OUTPUT_DIR}")
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${BIN_OUTPUT_DIR}")
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG "${BIN_OUTPUT_DIR}")
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE "${BIN_OUTPUT_DIR}")
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${BIN_OUTPUT_DIR}")
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${BIN_OUTPUT_DIR}")
endif()

file(GLOB_RECURSE PROC_COMMON ../Common/*.h ../Common/*.hpp)
source_group("proc_common" FILES ${PROC_COMMON})

file(GLOB_RECURSE THIRD_INCLUDE ../third_Include/*.h ../third_Include/*.hpp)
source_group("third_Include" FILES ${THIRD_INCLUDE})
	
file(GLOB_RECURSE COMMON_FILES ../PSS_ASIO/Common/*.h ../PSS_ASIO/Common/*.cpp)
source_group("Common" FILES ${COMMON_FILES})

file(GLOB_RECURSE MESSAGE_FILES ../PSS_ASIO/Message/*.h ../PSS_ASIO/Message/*.cpp)
source_group("Message" FILES ${MESSAGE_FILES})	

file(GLOB_RECURSE TCPSESSION_FILES ../PSS_ASIO/TcpSession/*.h ../PSS_ASIO/TcpSession/*.cpp)
source_group("TcpSession" FILES ${TCPSESSION_FILES})

file(GLOB_RECURSE UDPSESSION_FILES ../PSS_ASIO/UdpSession/*.h ../PSS_ASIO/UdpSession/*.cpp)
source_group("UdpSession" FILES ${UDPSESSION_FILES})

file(GLOB_RECURSE TTYSESSION_FILES ../PSS_ASIO/TTySession/*.h ../PSS_ASIO/TTySession/*.cpp)
source_group("TTySession" FILES ${TTYSESSION_FILES})
	
file(GLOB MAIN_FILES ${PROJECT_SOURCE_DIR}/*.cpp)
source_group("Main" FILES ${MAIN_FILES})

include_directories(
	../third_include
	../Common
	../PSS_ASIO/Common
	../PSS_ASIO/Message
	../PSS_ASIO/TcpSession
	../PSS_ASIO/UdpSession
	../PSS_ASIO/TTySession
	${GTEST_INCLUDE_DIR})

add_executable(test_pss_asio 
	${MAIN_FILES}
	${PROC_COMMON}
	${THIRD_INCLUDE}
	${COMMON_FILES}
	${MESSAGE_FILES}
	${TCPSESSION_FILES}
	${UDPSESSION_FILES}
	${TTYSESSION_FILES}
	)

#ifwindows	
#_DEBUG;_CONSOLE;_WIN32_WINDOWS;_WINSOCK_DEPRECATED_NO_WARNINGS;ASIO_HAS_SERIAL_PORT;ASIO_HAS_IOCP;
if(WIN32)
	message("OS is windows...")
	if(CMAKE_BUILD_TYPE MATCHES "debug")
		add_definitions(
		-D_DEBUG
		-D_CONSOLE
		-D_WIN32_WINDOWS
		-D_WINSOCK_DEPRECATED_NO_WARNINGS
		-DASIO_HAS_SERIAL_PORT
		-DASIO_HAS_IOCP)
	else()
		add_definitions(
		-DNDEBUG
		-D_CONSOLE
		-D_WIN32_WINDOWS
		-D_WINSOCK_DEPRECATED_NO_WARNINGS
		-DASIO_HAS_SERIAL_PORT
		-DASIO_HAS_IOCP)		
	endif()
endif()

if(UNIX)
	find_package(GTEST)
	if (!GTEST_FOUND)
    message(FATAL_ERROR "googletest library is needed but can't be found")
	endif ()
	
	link_directories(${GTEST_LIBRARY})
	target_link_libraries(test_pss_asio gtest pthread dl)
endif()

set_target_properties(test_pss_asio PROPERTIES 
	OUTPUT_NAME_DEBUG "test_pss_asio"
	ARCHIVE_OUTPUT_DIRECTORY ${BIN_OUTPUT_DIR}
    RUNTIME_OUTPUT_DIRECTORY ${BIN_OUTPUT_DIR}
    LIBRARY_OUTPUT_DIRECTORY ${BIN_OUTPUT_DIR})

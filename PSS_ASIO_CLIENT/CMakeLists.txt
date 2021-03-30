if(UNIX)
	set(BIN_OUTPUT_DIR ${ROOT_DIR}/Build/Linux/)
	set(LIB_OUTPUT_DIR ${ROOT_DIR}/Build/Linux/)
else()
	set(BIN_OUTPUT_DIR ${ROOT_DIR}/Build/Windows/)
	set(LIB_OUTPUT_DIR ${ROOT_DIR}/Build/Windows/)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${BIN_OUTPUT_DIR}")
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${BIN_OUTPUT_DIR}")
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG "${BIN_OUTPUT_DIR}")
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE "${BIN_OUTPUT_DIR}")
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${BIN_OUTPUT_DIR}")
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${BIN_OUTPUT_DIR}")
endif()

file(GLOB PROC_COMMON ${PROJECT_SOURCE_DIR}/COMMON/*.h ${PROJECT_SOURCE_DIR}/COMMON/*.hpp)
source_group("proc_common" FILES ${PROC_COMMON})

file(GLOB THIRD_INCLUDE ${PROJECT_SOURCE_DIR}/third_Include/*.h ${PROJECT_SOURCE_DIR}/third_Include/*.hpp)
source_group("third_Include" FILES ${THIRD_INCLUDE})

file(GLOB COMMON_FILES Common/*.h Common/*.cpp)
source_group("Common" FILES ${COMMON_FILES})

file(GLOB UDPSESSION_FILES UdpSession/*.h UdpSession/*.cpp)
source_group("udpsession" FILES ${MESSAGE_FILES})

file(GLOB TCPSESSION_FILES TcpSession/*.h TcpSession/*.cpp)
source_group("tcpsession" FILES ${MESSAGE_FILES})

file(GLOB_RECURSE MAIN_FILES ${PROJECT_SOURCE_DIR}/PSS_ASIO_CLIENT.cpp)
source_group("Main" FILES ${MAIN_FILES})

include_directories(
	${PROJECT_SOURCE_DIR}/third_include
	${PROJECT_SOURCE_DIR}/Common
	Common
	UdpSession
	TcpSession)

add_executable(pss_asio_test_client 
	${MAIN_FILES}
	${PROC_COMMON}
	${THIRD_INCLUDE}
	${COMMON_FILES}
	${TCPSESSION_FILES}
	${UDPSESSION_FILES}
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
	message("OS is Linux...")
	if(gcov_check STREQUAL "1")
		message("gcov_check is set...")
		set(CMAKE_CXX_FLAGS   "${CMAKE_CXX_FLAGS} -fprofile-arcs")
		set(CMAKE_CXX_FLAGS   "${CMAKE_CXX_FLAGS} -ftest-coverage")
	endif()
	
	if(CMAKE_BUILD_TYPE MATCHES "debug")
		add_definitions(
		-DASIO_DISABLE_NOEXCEPT)
	else()
		add_definitions(
		-DASIO_DISABLE_NOEXCEPT)		
	endif()

	if(gcov_check STREQUAL "1")
		target_link_libraries(pss_asio_test_client -lpthread -ldl -lgcov)
	else()
		target_link_libraries(pss_asio_test_client -lpthread -ldl)
	endif()
endif()

set_target_properties(pss_asio_test_client PROPERTIES 
	OUTPUT_NAME_DEBUG "pss_asio_test_client"
	ARCHIVE_OUTPUT_DIRECTORY ${BIN_OUTPUT_DIR}
    RUNTIME_OUTPUT_DIRECTORY ${BIN_OUTPUT_DIR}
    LIBRARY_OUTPUT_DIRECTORY ${BIN_OUTPUT_DIR})

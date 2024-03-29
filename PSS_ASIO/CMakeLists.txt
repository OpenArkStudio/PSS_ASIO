if(UNIX)
	set(BIN_OUTPUT_DIR ${ROOT_DIR}/Build/Linux/)
	set(LIB_OUTPUT_DIR ${ROOT_DIR}/Build/Linux/)
	
	if (NOT ${libjemalloc} STREQUAL libjemalloc-NOTFOUND)
		set(jemalloc_path ${PROJECT_SOURCE_DIR}/Build/Linux/commonlib)
		link_directories(${jemalloc_path})
	endif()	
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

file(GLOB THIRD_INCLUDE ${PROJECT_SOURCE_DIR}/third_include/*.h ${PROJECT_SOURCE_DIR}/third_include/*.hpp)
source_group("third_Include" FILES ${THIRD_INCLUDE})

file(GLOB KCP_FILES ${PROJECT_SOURCE_DIR}/third_include/kcp/*.h ${PROJECT_SOURCE_DIR}/third_include/kcp/*.c)
source_group("kcp" FILES ${KCP_FILES})

file(GLOB COMMON_FILES Common/*.h Common/*.cpp)
source_group("Common" FILES ${COMMON_FILES})

file(GLOB MESSAGE_FILES Message/*.h Message/*.cpp)
source_group("Message" FILES ${MESSAGE_FILES})

file(GLOB TCPSESSION_FILES TcpSession/*.h TcpSession/*.cpp)
source_group("TcpSession" FILES ${TCPSESSION_FILES})

file(GLOB UDPSESSION_FILES UdpSession/*.h UdpSession/*.cpp)
source_group("UdpSession" FILES ${UDPSESSION_FILES})

file(GLOB TTYSESSION_FILES TTySession/*.h TTySession/*.cpp)
source_group("TTySession" FILES ${TTYSESSION_FILES})

file(GLOB QUEUESESSION_FILES QueueSession/*.h QueueSession/*.cpp)
source_group("QueueSession" FILES ${QUEUESESSION_FILES})

file(GLOB_RECURSE MAIN_FILES ${PROJECT_SOURCE_DIR}/PSS_ASIO.cpp)
source_group("Main" FILES ${MAIN_FILES})

include_directories(
	${PROJECT_SOURCE_DIR}/third_include
	${PROJECT_SOURCE_DIR}/Common
	Message
	TcpSession
	Common
	UdpSession
	TTySession
	QueueSession)

add_executable(pss_asio 
	${MAIN_FILES}
	${KCP_FILES}
	${PROC_COMMON}
	${THIRD_INCLUDE}
	${COMMON_FILES}
	${MESSAGE_FILES}
	${TCPSESSION_FILES}
	${UDPSESSION_FILES}
	${TTYSESSION_FILES}
	${QUEUESESSION_FILES}
	)

if(WIN32)
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
		message("[pss_asio]gcov_check is set...")
		set(CMAKE_CXX_FLAGS " ${CMAKE_CXX_FLAGS} --coverage")
		message("[pss_asio]gcov_check is set...done")
	endif()
	
	if(CMAKE_BUILD_TYPE MATCHES "debug")
		add_definitions(
		-DASIO_DISABLE_NOEXCEPT)
	else()
		add_definitions(
		-DASIO_DISABLE_NOEXCEPT)		
	endif()

	if(gcov_check STREQUAL "1")
		message("[PSS_ASIO]set GCOV_TEST...")
		add_definitions(-DGCOV_TEST)
		target_link_libraries(pss_asio -lpthread -ldl -lgcov)
	else()
		if(ssl_support STREQUAL "1")
			add_definitions(-DSSL_SUPPORT)
			target_link_libraries(pss_asio -lpthread -ldl -lssl -lcrypto)
		else()
			target_link_libraries(pss_asio -lpthread -ldl)
		endif()
	endif()
	
	#message("[pss main]libjemalloc= ${libjemalloc}")
	if (${libjemalloc} STREQUAL libjemalloc-NOTFOUND)
		message("[pss main] jemalloc is not find")
	else()
		set(jemalloc_path ${PROJECT_SOURCE_DIR}/Build/Linux/commonlib)
		message("[pss main] jemalloc path= ${jemalloc_path}")
		link_directories(${jemalloc_path})
		target_link_libraries(pss_asio -ljemalloc)
	endif()
endif()

set_target_properties(pss_asio PROPERTIES 
	OUTPUT_NAME_DEBUG "pss_asio"
	ARCHIVE_OUTPUT_DIRECTORY ${BIN_OUTPUT_DIR}
    RUNTIME_OUTPUT_DIRECTORY ${BIN_OUTPUT_DIR}
    LIBRARY_OUTPUT_DIRECTORY ${BIN_OUTPUT_DIR})

PROJECT(PacketParse_Interface)

if(UNIX)
	set(BIN_OUTPUT_DIR ${ROOT_DIR}/Build/Linux/)
	set(LIB_OUTPUT_DIR ${ROOT_DIR}/Build/Linux/)
	
	if (NOT ${libjemalloc} STREQUAL libjemalloc-NOTFOUND)
		set(jemalloc_path ${PROJECT_SOURCE_DIR}/../Build/Linux/commonlib)
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

file(GLOB MAIN_FILES ${PROJECT_SOURCE_DIR}/*.cpp)
source_group("main_file" FILES ${MAIN_FILES})

include_directories(
	${PROJECT_SOURCE_DIR}/../Common
	${PROJECT_SOURCE_DIR}/../third_include)

ADD_LIBRARY(${PROJECT_NAME} SHARED 
	${MAIN_FILES}
	)

if(WIN32)
	if(CMAKE_BUILD_TYPE MATCHES "debug")
		add_definitions(
		-D_DEBUG
		-D_USRDLL
		-DPACKETPARSE_INTERFACE_EXPORTS
		-D_WINDOWS)
	else()
		add_definitions(
		-DNDEBUG
		-D_USRDLL
		-DPACKETPARSE_INTERFACE_EXPORTS
		-D_WINDOWS)		
	endif()
endif()

if(UNIX)
	target_link_libraries(${PROJECT_NAME} -lpthread)
	
	if (${libjemalloc} STREQUAL libjemalloc-NOTFOUND)
		message("[packet parse] jemalloc is not find")
	else()
		set(jemalloc_path ${PROJECT_SOURCE_DIR}/Build/Linux/commonlib)
		message("[packet parse] jemalloc path= ${jemalloc_path}")
		target_link_libraries(${PROJECT_NAME}  -ljemalloc)
	endif()		
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES 
	OUTPUT_NAME_DEBUG ${PROJECT_NAME}
	ARCHIVE_OUTPUT_DIRECTORY ${BIN_OUTPUT_DIR}
    RUNTIME_OUTPUT_DIRECTORY ${BIN_OUTPUT_DIR}
    LIBRARY_OUTPUT_DIRECTORY ${BIN_OUTPUT_DIR})

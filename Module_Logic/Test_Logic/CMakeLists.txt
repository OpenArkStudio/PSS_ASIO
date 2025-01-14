PROJECT(Test_Logic)

if(UNIX)
	set(BIN_OUTPUT_DIR ${ROOT_DIR}/Build/Linux/)
	set(LIB_OUTPUT_DIR ${ROOT_DIR}/Build/Linux/)
	
	if (NOT ${libjemalloc} STREQUAL libjemalloc-NOTFOUND)
		set(jemalloc_path ${PROJECT_SOURCE_DIR}/../../Build/Linux/commonlib)
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

file(GLOB MAIN_FILES *.cpp)
source_group("main_file" FILES ${MAIN_FILES})

include_directories(
	${PROJECT_SOURCE_DIR}/../../Common
	${PROJECT_SOURCE_DIR}/../../third_include)

ADD_LIBRARY(${PROJECT_NAME} SHARED 
	${MAIN_FILES}
	)

#ifwindows	
#__DEBUG;_USRDLL;TEST_LOGIC_EXPORTS;_WINDOWS;%(PreprocessorDefinitions)
if(WIN32)
	message("OS is windows...")
	if(CMAKE_BUILD_TYPE MATCHES "debug")
		add_definitions(
		-D_DEBUG
		-D_USRDLL
		-DTEST_LOGIC_EXPORTS
		-D_WINDOWS)
	else()
		add_definitions(
		-DNDEBUG
		-D_USRDLL
		-DTEST_LOGIC_EXPORTS
		-D_WINDOWS)		
	endif()
endif()

if(gcov_check STREQUAL "1")
	message("[Test_Logic]set GCOV_TEST...")
	add_definitions(-DGCOV_TEST)
endif()

if(UNIX)
	target_link_libraries(${PROJECT_NAME} -lpthread)
	
	if (${libjemalloc} STREQUAL libjemalloc-NOTFOUND)
		message("[logic] jemalloc is not find")
	else()
		set(jemalloc_path ${PROJECT_SOURCE_DIR}/Build/Linux/commonlib)
		message("[logic] jemalloc path= ${jemalloc_path}")
		target_link_libraries(${PROJECT_NAME}  -ljemalloc)
	endif()	
	
	if(gcov_check STREQUAL "1")
		message("[logic]gcov_check is set...")
		# set(CMAKE_CXX_FLAGS " ${CMAKE_CXX_FLAGS} --coverage")
		# 编译选项
        add_compile_options(
            -fprofile-arcs
            -ftest-coverage
            -fprofile-update=atomic
        )

        # 链接选项
        add_link_options(
            --coverage
        )		
		message("[logic]gcov_check is set...done")
	endif()	
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES 
	OUTPUT_NAME_DEBUG ${PROJECT_NAME}
	ARCHIVE_OUTPUT_DIRECTORY ${BIN_OUTPUT_DIR}
    RUNTIME_OUTPUT_DIRECTORY ${BIN_OUTPUT_DIR}
    LIBRARY_OUTPUT_DIRECTORY ${BIN_OUTPUT_DIR})

cmake_minimum_required(VERSION 3.0)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(json11)
add_subdirectory(tgbot-cpp)
add_subdirectory(googletest)

# Httplib

find_package(Python REQUIRED)
execute_process(COMMAND ${Python_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/cpp-httplib/split.py"
	WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/cpp-httplib"
	RESULT_VARIABLE split_ret
)

if(split_ret EQUAL 0)
	message(STATUS "split.py successfully split httplib")
else()
	message(FATAL_ERROR "split.py failed!")
endif()

file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/cpp-httplib/out/" DESTINATION "${CMAKE_BINARY_DIR}/httplib")
file(REMOVE_RECURSE "${CMAKE_CURRENT_SOURCE_DIR}/cpp-httplib/out")

add_library(Httplib STATIC "${CMAKE_BINARY_DIR}/httplib/httplib.cc")

set_target_properties(Httplib
	PROPERTIES
	CXX_STANDARD 17
	CXX_STANDARD_REQUIRED ON
	CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fno-rtti -O2"
)

target_include_directories(Httplib PUBLIC "${CMAKE_BINARY_DIR}/httplib")
target_link_libraries(Httplib ${CMAKE_THREAD_LIBS_INIT})

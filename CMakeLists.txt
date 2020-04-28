cmake_minimum_required(VERSION 3.0)
project(Foodculator)

add_executable(foodculator main.cpp)

execute_process(COMMAND python "${CMAKE_CURRENT_SOURCE_DIR}/cpp-httplib/split.py"
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/cpp-httplib/)

execute_process(COMMAND mv "${CMAKE_CURRENT_SOURCE_DIR}/cpp-httplib/out" "${CMAKE_BINARY_DIR}/httplib")

set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_library(Httplib STATIC "${CMAKE_BINARY_DIR}/httplib/httplib.cc")

set_target_properties(foodculator Httplib
	PROPERTIES
	CXX_STANDARD 17
	CXX_STANDARD_REQUIRED ON
)

SET(CMAKE_CXX_FLAGS "-Wall -fno-rtti -O2" )

add_subdirectory(db)
add_subdirectory(json11)

target_link_libraries(foodculator DbLib 
								  Httplib 
								  json11 
								  pthread)

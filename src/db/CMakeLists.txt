cmake_minimum_required(VERSION 3.0)

add_library(DbLib STATIC db.cpp)

set_target_properties(DbLib
	PROPERTIES
	CXX_STANDARD 17
	CXX_STANDARD_REQUIRED ON
	CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fno-rtti -O2"
)

include_directories("${PROJECT_SOURCE_DIR}/src" "${PROJECT_SOURCE_DIR}/lib")

target_link_libraries(DbLib sqlite3 json11)

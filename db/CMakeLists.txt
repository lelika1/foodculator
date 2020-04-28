cmake_minimum_required(VERSION 3.0)

add_library(DbLib STATIC db.cpp)

set_target_properties(DbLib
	PROPERTIES
	CXX_STANDARD 17
	CXX_STANDARD_REQUIRED ON
)

target_link_libraries(DbLib sqlite3)
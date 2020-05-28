cmake_minimum_required(VERSION 3.0)

add_library(UtilLib STATIC statusor.cpp)

set_target_properties(UtilLib
	PROPERTIES
	CXX_STANDARD 17
	CXX_STANDARD_REQUIRED ON
	CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fno-rtti -O2"
)


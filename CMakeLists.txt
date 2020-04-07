cmake_minimum_required(VERSION 3.0)
project(Foodculator)

add_executable(foodculator main.cpp)

set_target_properties(foodculator
	PROPERTIES
	CXX_STANDARD 17
	CXX_STANDARD_REQUIRED ON
)

add_subdirectory(db)

target_link_libraries(foodculator DbLib)
target_link_libraries(foodculator pthread)

cmake_minimum_required(VERSION 3.0)

add_executable(foodculator main.cpp)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set_target_properties(foodculator
	PROPERTIES
	CXX_STANDARD 17
	CXX_STANDARD_REQUIRED ON
	CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fno-rtti -O2"
)

add_subdirectory(db)
add_subdirectory(util)

include_directories("${PROJECT_SOURCE_DIR}/src" "${PROJECT_SOURCE_DIR}/lib")
target_link_libraries(foodculator DbLib
								  fmt
								  UtilLib 
								  Httplib 
								  json11 
								  TgBot)

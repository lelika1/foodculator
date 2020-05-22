cmake_minimum_required(VERSION 3.0)

add_executable(tests main.cpp db.cpp)
add_test(NAME tests COMMAND tests)

set_target_properties(tests
	PROPERTIES
	CXX_STANDARD 17
	CXX_STANDARD_REQUIRED ON
	CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fno-rtti -O2"
)

target_include_directories(tests PRIVATE "${PROJECT_SOURCE_DIR}")
target_link_libraries(tests DbLib gtest)

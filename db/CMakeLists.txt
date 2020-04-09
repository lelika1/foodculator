cmake_minimum_required(VERSION 3.0)

add_library(DbLib STATIC db.cpp)

target_link_libraries(DbLib sqlite3)
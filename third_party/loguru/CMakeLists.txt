cmake_minimum_required(VERSION 3.16)
project(loguru)

add_library(loguru STATIC loguru.cpp)
set_target_properties(loguru PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(loguru PUBLIC .)

# On Linux, loguru requires pthread and dl (for stacktrace/thread naming)
if(UNIX)
    find_package(Threads REQUIRED)
    target_link_libraries(loguru PUBLIC Threads::Threads ${CMAKE_DL_LIBS})
endif()
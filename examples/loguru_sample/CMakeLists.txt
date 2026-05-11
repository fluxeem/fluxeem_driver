cmake_minimum_required(VERSION 3.16)
project(loguru_sample)

set(CMAKE_CXX_STANDARD 20)

if(MSVC)
    add_compile_options(/utf-8 /wd4251)
endif()

if(FLUXEEM_DRIVER_BUILDING)
    add_executable(loguru_sample loguru_sample.cpp)
    target_link_libraries(loguru_sample PRIVATE loguru)
else()
    include(FetchContent)
    FetchContent_Declare(
        loguru
        GIT_REPOSITORY https://github.com/emilk/loguru.git
        GIT_TAG master
    )
    FetchContent_MakeAvailable(loguru)

    add_executable(loguru_sample loguru_sample.cpp)
    target_link_libraries(loguru_sample PRIVATE loguru)
endif()

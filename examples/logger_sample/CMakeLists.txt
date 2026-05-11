cmake_minimum_required(VERSION 3.10)
project(logger_sample)

set(CMAKE_CXX_STANDARD 20)

if(MSVC)
    add_compile_options(/utf-8 /wd4251)
endif()

if(NOT FLUXEEM_DRIVER_BUILDING)
    if(DEFINED ENV{FLUXEEM_DRIVER_ROOT})
        list(PREPEND CMAKE_PREFIX_PATH "$ENV{FLUXEEM_DRIVER_ROOT}")
    endif()

    find_package(fluxeem_driver CONFIG REQUIRED)

    if(NOT TARGET fluxeem::fluxeem_driver)
        message(FATAL_ERROR "fluxeem::fluxeem_driver target not found in installed package")
    endif()
endif()

add_executable(logger_sample logger_sample.cpp)

if(FLUXEEM_DRIVER_BUILDING)
    target_link_libraries(logger_sample PRIVATE fluxeem_driver)
else()
    target_link_libraries(logger_sample PRIVATE fluxeem::fluxeem_driver)
endif()

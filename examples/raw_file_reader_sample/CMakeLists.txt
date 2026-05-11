cmake_minimum_required(VERSION 3.10)
project(raw_file_reader_sample)

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

find_package(OpenCV REQUIRED)

add_executable(raw_file_reader_sample raw_file_reader_sample.cpp)

if(FLUXEEM_DRIVER_BUILDING)
    target_link_libraries(raw_file_reader_sample PRIVATE 
        fluxeem_driver 
        ${OpenCV_LIBS}
    )
else()
    target_link_libraries(raw_file_reader_sample PRIVATE 
        fluxeem::fluxeem_driver 
        ${OpenCV_LIBS}
    )
endif()

target_include_directories(raw_file_reader_sample
PRIVATE
    ${OpenCV_INCLUDE_DIRS}
)

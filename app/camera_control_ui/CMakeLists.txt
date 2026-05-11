cmake_minimum_required(VERSION 3.10)
project(camera_control_ui)

set(CMAKE_CXX_STANDARD 20)

if(MSVC)
    add_compile_options(/utf-8 /wd4251)
endif()

if(NOT FLUXEEM_DRIVER_BUILDING)
    find_package(fluxeem_driver REQUIRED)
endif()

find_package(OpenCV REQUIRED)

add_executable(camera_control_ui camera_control_ui.cpp)

if(FLUXEEM_DRIVER_BUILDING)
    target_link_libraries(camera_control_ui PRIVATE 
        fluxeem_driver 
        ${OpenCV_LIBS}
    )
else()
    target_link_libraries(camera_control_ui PRIVATE 
        fluxeem::fluxeem_driver 
        ${OpenCV_LIBS}
    )
endif()

target_include_directories(camera_control_ui
PRIVATE
    ${OpenCV_INCLUDE_DIRS}
)

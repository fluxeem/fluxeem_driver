# CMakeLists.txt for src/hal/sensor directory

target_sources(${LibName} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/sensor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imx6x6_sensor.cpp
)

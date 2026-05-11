# CMakeLists.txt for src/hal/tools/biases directory

target_sources(${LibName} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/imx636_biases.cpp
)

# CMakeLists.txt for src/hal/tools/trigger directory

target_sources(${LibName} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/imx636_trigger_in.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imx636_sync.cpp
)

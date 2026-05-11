# CMakeLists.txt for src/hal/registers directory

target_sources(${LibName} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/register.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/register_controller.cpp
)

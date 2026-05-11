# CMakeLists.txt for src/driver/file_reader directory

target_sources(${LibName} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/ev_file_reader.cpp
    ${CMAKE_CURRENT_LIST_DIR}/raw_file_reader.cpp
)

# Tests CMakeLists.txt

# Find Google Test from vcpkg
find_package(GTest CONFIG REQUIRED)

# Test executable
set(TEST_NAME ${PROJECT_NAME}_tests)

add_executable(${TEST_NAME}
    test_main.cpp
    test_event_types.cpp
    test_camera_tools.cpp
    test_object_pool.cpp
    test_camera_hardware.cpp
    test_camera_basic.cpp
    test_camera_config.cpp
    test_file_reader.cpp
)

target_link_libraries(${TEST_NAME}
    PRIVATE
        ${PROJECT_NAME}
        GTest::gtest_main
        GTest::gtest
        loguru
)

target_include_directories(${TEST_NAME}
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${LIBUSB_INCLUDE_DIR}
)

# Add tests to CTest
add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})

# Set test properties
set_target_properties(${TEST_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Copy test data if needed
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/test_data)
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/test_data DESTINATION ${CMAKE_BINARY_DIR}/bin)
endif()

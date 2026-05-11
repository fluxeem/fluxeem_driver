
# Set the library name variable for subdirectories
set(LibName ${PROJECT_NAME})

# Add subdirectories
add_subdirectory(base)
add_subdirectory(hal)
add_subdirectory(driver)

# Add sources to parent scope
set(SOURCES ${SOURCES} PARENT_SCOPE)

target_sources(${LibName} PRIVATE
	${CMAKE_CURRENT_LIST_DIR}/tool_info.cpp
	${CMAKE_CURRENT_LIST_DIR}/camera_tool.cpp
	${CMAKE_CURRENT_LIST_DIR}/camera_tool_private.cpp
)

add_subdirectory(biases)
add_subdirectory(trigger)
add_subdirectory(anti_flicker)
add_subdirectory(roi)
add_subdirectory(event_rate_control)
add_subdirectory(event_trail_filter)
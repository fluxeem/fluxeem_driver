# CMakeLists.txt for src/hal/common directory

target_sources(${LibName} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/interface.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/usb_device.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/usb_interface.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/async_bulk_transfer_handle.cpp
)

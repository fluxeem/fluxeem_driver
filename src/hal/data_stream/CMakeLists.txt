# CMakeLists.txt for src/hal/data_stream directory

target_sources(${LibName} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/data_transfer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/decoder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/decoder_factory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/evt3_decoder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/raw_event_stream_encoding_type.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/usb_data_transfer.cpp
)

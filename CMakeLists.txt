cmake_minimum_required(VERSION 3.10)

set(PROJECT_VERSION_MAJOR 1)
set(PROJECT_VERSION_MINOR 0)
set(PROJECT_VERSION_PATCH 1)
set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
string(TIMESTAMP BUILD_DATE "%Y-%m-%d %H:%M:%S")
message(STATUS "Build date: ${BUILD_DATE}")
add_definitions(-DPROJECT_VERSION="${PROJECT_VERSION}")
add_definitions(-DBUILD_DATE="${BUILD_DATE}")
project(fluxeem_driver VERSION ${PROJECT_VERSION})
set(CMAKE_CXX_STANDARD 20)
add_definitions(-D NOMINMAX)
if(MSVC)
    add_compile_options(/utf-8 /wd4251 /FS)
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/SetupVcpkgFindHints.cmake)

option(BUILD_TESTING "Build tests" OFF)
option(GENERATE_DOCS "Auto documentation generation" OFF)
option(BUILD_SAMPLES "Build sample applications" OFF)
set(DOXYGEN_AWESOME_DIR "" CACHE PATH "Root directory of doxygen-awesome-css repository")


if(NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
endif()
if(NOT CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
endif()
if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endif()

# Set the output directory for the build
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(INSTALL_INCLUDE_DIR include)
set(INSTALL_BINARY_DIR bin)
set(INSTALL_LIB_DIR lib)
set(INSTALL_DOC_DIR docs/${PROJECT_NAME})
set(INSTALL_SHARE_DIR share)

set(CMAKE_DEBUG_POSTFIX d)

add_subdirectory(third_party/loguru)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/modules)
find_package(LibUSB MODULE REQUIRED)

# Create the library target first
add_library(${PROJECT_NAME} SHARED)

# Define FLUXEEM_BUILDING_DLL when building the library
target_compile_definitions(${PROJECT_NAME} PRIVATE FLUXEEM_BUILDING_DLL)

# Add source files from subdirectories
add_subdirectory(src)

target_link_libraries(${PROJECT_NAME} PRIVATE loguru ${LIBUSB_LIBRARIES})

target_include_directories(${PROJECT_NAME}
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
        $<INSTALL_INTERFACE:${INSTALL_INCLUDE_DIR}>
    PRIVATE
        ${LIBUSB_INCLUDE_DIR}
)

# ══════════════════════════════════════════════════════════════════
#  安装规则
# ══════════════════════════════════════════════════════════════════

install(TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}Targets
    RUNTIME DESTINATION ${INSTALL_BINARY_DIR}
    LIBRARY DESTINATION ${INSTALL_LIB_DIR}
    ARCHIVE DESTINATION ${INSTALL_LIB_DIR}
)

if(WIN32)
    install(FILES $<TARGET_RUNTIME_DLLS:${PROJECT_NAME}>
        DESTINATION ${INSTALL_BINARY_DIR}
        OPTIONAL
    )
endif()

if(WIN32 AND LIBUSB_RUNTIME_LIBRARIES)
    install(FILES ${LIBUSB_RUNTIME_LIBRARIES}
        DESTINATION ${INSTALL_BINARY_DIR}
        OPTIONAL
    )
endif()

# 仅安装公开 API 头文件
set(FLUXEEM_PUBLIC_API_HEADERS
    include/fluxeem/base/logging/logger.h
    include/fluxeem/base/define/base_define.h
    include/fluxeem/base/define/camera_types.h
    include/fluxeem/base/define/event_type.h
    include/fluxeem/base/utility/json/json.hpp
    include/fluxeem/hal/tools/camera_tool.h
    include/fluxeem/hal/tools/tool_info.h
    include/fluxeem/hal/tools/param_descriptor.h
    include/fluxeem/driver/camera/ev_camera_service.hpp
    include/fluxeem/driver/camera/base/ev_camera_service.hpp
    include/fluxeem/driver/camera/base/i_camera.hpp
    include/fluxeem/driver/file_reader/ev_file_reader.h
)

foreach(api_header IN LISTS FLUXEEM_PUBLIC_API_HEADERS)
    get_filename_component(api_header_dir ${api_header} DIRECTORY)
    string(REPLACE "include" "${INSTALL_INCLUDE_DIR}" api_install_dir ${api_header_dir})
    install(FILES ${api_header}
        DESTINATION ${api_install_dir}
    )
endforeach()

include(CMakePackageConfigHelpers)

install(EXPORT ${PROJECT_NAME}Targets
    FILE ${PROJECT_NAME}Targets.cmake
    NAMESPACE fluxeem::
    DESTINATION ${INSTALL_SHARE_DIR}/cmake/${PROJECT_NAME}
)

configure_package_config_file(
    cmake/${PROJECT_NAME}Config.cmake.in
    ${CMAKE_BINARY_DIR}/${PROJECT_NAME}Config.cmake
    INSTALL_DESTINATION ${INSTALL_SHARE_DIR}/cmake/${PROJECT_NAME}
)

write_basic_package_version_file(
    ${CMAKE_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_BINARY_DIR}/${PROJECT_NAME}Config.cmake
    ${CMAKE_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
    DESTINATION ${INSTALL_SHARE_DIR}/cmake/${PROJECT_NAME}
)

install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE
    DESTINATION ${INSTALL_SHARE_DIR}/licenses/${PROJECT_NAME}
)

if(UNIX AND NOT APPLE AND NOT ANDROID)
    option(UDEV_RULES_SYSTEM_INSTALL "Install udev rules to /etc/udev/rules.d/ (requires root)" ON)
    if(UDEV_RULES_SYSTEM_INSTALL)
        install(FILES installer/ubuntu/fluxeem.rules
            DESTINATION /etc/udev/rules.d
        )
        install(CODE "
            execute_process(COMMAND udevadm control --reload-rules ERROR_QUIET)
            execute_process(COMMAND udevadm trigger ERROR_QUIET)
        ")
    else()
        install(FILES installer/ubuntu/fluxeem.rules
            DESTINATION ${INSTALL_SHARE_DIR}/${PROJECT_NAME}/udev
        )
    endif()
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/installer/ubuntu/scripts/install_usb_driver.sh")
        install(PROGRAMS
            installer/ubuntu/scripts/install_usb_driver.sh
            DESTINATION ${INSTALL_SHARE_DIR}/${PROJECT_NAME}/scripts/udev
        )
    endif()
endif()

if(WIN32)
    install(FILES
        installer/windows/install_driver.bat
        installer/windows/uninstall_driver.bat
        DESTINATION installer
    )
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/installer/windows/usb_driver")
        install(DIRECTORY installer/windows/usb_driver
            DESTINATION installer
        )
    endif()
endif()

# ══════════════════════════════════════════════════════════════════
#  示例程序
# ══════════════════════════════════════════════════════════════════
if(BUILD_SAMPLES)
    set(FLUXEEM_DRIVER_BUILDING ON)
    add_subdirectory(examples/loguru_sample)
    add_subdirectory(examples/logger_sample)
    add_subdirectory(examples/fluxeem_live_viewer)
    add_subdirectory(examples/raw_file_reader_sample)
    add_subdirectory(examples/slow_motion_playback_sample)
    add_subdirectory(examples/camera_hardware_sync_sample)
    set(FLUXEEM_SAMPLE_TARGETS
        fluxeem_live_viewer
        raw_file_reader_sample
        slow_motion_playback_sample
        camera_hardware_sync_sample
    )
    set(FLUXEEM_SAMPLE_RUNTIME_DEP_TARGETS ${FLUXEEM_SAMPLE_TARGETS})
    if(UNIX AND NOT APPLE)
        set(FLUXEEM_SAMPLE_INSTALL_BIN "/opt/fluxeem/bin")
        set_target_properties(${FLUXEEM_SAMPLE_TARGETS} PROPERTIES
            INSTALL_RPATH "/opt/fluxeem/lib"
        )
    else()
        set(FLUXEEM_SAMPLE_INSTALL_BIN ${INSTALL_BINARY_DIR})
    endif()

    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/app/camera_control_ui/CMakeLists.txt")
        add_subdirectory(app/camera_control_ui)
        if(UNIX AND NOT APPLE)
            set_target_properties(camera_control_ui PROPERTIES INSTALL_RPATH "/opt/fluxeem/lib")
        endif()
        install(TARGETS camera_control_ui RUNTIME DESTINATION ${FLUXEEM_SAMPLE_INSTALL_BIN})
        list(APPEND FLUXEEM_SAMPLE_RUNTIME_DEP_TARGETS camera_control_ui)
    else()
        message(WARNING "app/camera_control_ui directory not found. Skipping camera_control_ui build.")
    endif()

    install(TARGETS ${FLUXEEM_SAMPLE_TARGETS}
        RUNTIME DESTINATION ${FLUXEEM_SAMPLE_INSTALL_BIN}
    )

    if(WIN32 AND CMAKE_VERSION VERSION_GREATER_EQUAL "3.21")
        foreach(sample_target IN LISTS FLUXEEM_SAMPLE_RUNTIME_DEP_TARGETS)
            install(FILES $<TARGET_RUNTIME_DLLS:${sample_target}>
                DESTINATION ${INSTALL_BINARY_DIR}
                OPTIONAL
            )
        endforeach()

        cmake_policy(SET CMP0087 NEW)
        if(OpenCV_DIR)
            get_filename_component(_extra_dll_dir "${OpenCV_DIR}/../../bin" ABSOLUTE)
            set(_exe_names "")
            foreach(_t IN LISTS FLUXEEM_SAMPLE_RUNTIME_DEP_TARGETS)
                list(APPEND _exe_names "$<TARGET_FILE_NAME:${_t}>")
            endforeach()
            install(CODE "
                cmake_policy(SET CMP0087 NEW)
                if(POLICY CMP0207)
                    cmake_policy(SET CMP0207 NEW)
                endif()
                set(_bin \"\${CMAKE_INSTALL_PREFIX}/${INSTALL_BINARY_DIR}\")
                set(_exes \"\")
                foreach(_n IN ITEMS ${_exe_names})
                    if(EXISTS \"\${_bin}/\${_n}\")
                        list(APPEND _exes \"\${_bin}/\${_n}\")
                    endif()
                endforeach()
                if(_exes)
                    file(GET_RUNTIME_DEPENDENCIES
                        EXECUTABLES \${_exes}
                        RESOLVED_DEPENDENCIES_VAR   _r
                        UNRESOLVED_DEPENDENCIES_VAR _u
                        DIRECTORIES \"${_extra_dll_dir}\" \"\${_bin}\"
                        PRE_EXCLUDE_REGEXES  [[api-ms-win-.*]] [[ext-ms-.*]]
                        POST_EXCLUDE_REGEXES [[.*[Ss]ystem32[/\\\\].*]] [[.*[Ss]ysWOW64[/\\\\].*]]
                    )
                    foreach(_d \${_r})
                        file(INSTALL \"\${_d}\" DESTINATION \"\${_bin}\" FOLLOW_SYMLINK_CHAIN)
                    endforeach()
                    if(_u)
                        message(WARNING \"Unresolved runtime deps: \${_u}\")
                    endif()
                endif()
            ")
        endif()
    endif()

    set(EXAMPLE_SAMPLES
        camera_hardware_sync_sample
        fluxeem_live_viewer
        raw_file_reader_sample
        slow_motion_playback_sample
    )
    install(DIRECTORY "app/camera_control_ui"
        DESTINATION examples
        FILES_MATCHING
            PATTERN "*.cpp"
            PATTERN "*.hpp"
            PATTERN "*.h"
            PATTERN "CMakeLists.txt"
    )
    foreach(sample IN LISTS EXAMPLE_SAMPLES)
        install(DIRECTORY "examples/${sample}"
            DESTINATION examples
            FILES_MATCHING
                PATTERN "*.cpp"
                PATTERN "*.hpp"
                PATTERN "*.h"
                PATTERN "CMakeLists.txt"
        )
    endforeach()
endif()

if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()

if(GENERATE_DOCS)
    if(NOT DOXYGEN_AWESOME_DIR)
        message(FATAL_ERROR
            "GENERATE_DOCS is ON, but DOXYGEN_AWESOME_DIR is not set. "
            "Please set it to the doxygen-awesome-css repository root, e.g. "
            "-DDOXYGEN_AWESOME_DIR=E:/Libs/doxygen-awesome-css"
        )
    endif()

    cmake_path(NORMAL_PATH DOXYGEN_AWESOME_DIR)
    set(DOXYGEN_AWESOME_CSS "${DOXYGEN_AWESOME_DIR}/doxygen-awesome.css")
    if(NOT EXISTS "${DOXYGEN_AWESOME_CSS}")
        message(FATAL_ERROR
            "DOXYGEN_AWESOME_DIR is invalid: '${DOXYGEN_AWESOME_DIR}'. "
            "Expected file not found: '${DOXYGEN_AWESOME_CSS}'"
        )
    endif()

    find_package(Doxygen OPTIONAL_COMPONENTS dot)
    if(DOXYGEN_FOUND)
        set(DOXYGEN_INPUT_DIR  "${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/docs ${PROJECT_SOURCE_DIR}/docs/tutorials")
        set(DOXYGEN_OUTPUT_DIR "${CMAKE_BINARY_DIR}/docs")

        if(Doxygen_dot_FOUND)
            set(DOXYGEN_HAVE_DOT YES)
        else()
            set(DOXYGEN_HAVE_DOT NO)
        endif()

        configure_file(
            "${PROJECT_SOURCE_DIR}/cmake/Doxyfile.in"
            "${CMAKE_BINARY_DIR}/Doxyfile"
            @ONLY
        )

        add_custom_target(docs
            COMMAND ${DOXYGEN_EXECUTABLE} "${CMAKE_BINARY_DIR}/Doxyfile"
            WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM
        )

        install(
            DIRECTORY "${CMAKE_BINARY_DIR}/docs/html"
            DESTINATION "${INSTALL_DOC_DIR}"
            OPTIONAL
        )

        message(STATUS "Doxygen found: ${DOXYGEN_VERSION}. Build target 'docs' is available.")
    else()
        message(WARNING "GENERATE_DOCS is ON but Doxygen was not found. Documentation will not be generated.")
    endif()
endif()

set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_VENDOR "Fluxeem")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Fluxeem Driver - Event Camera Driver and SDK.")
set(CPACK_PACKAGE_INSTALL_DIRECTORY ${PROJECT_NAME})
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_CONTACT "sdk@fluxeem.com")
set(CPACK_VERBATIM_VARIABLES ON)

if(WIN32)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64")
    else()
        set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES")
    endif()

    if(MSVC)
        set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION ${INSTALL_BINARY_DIR})
        set(CMAKE_INSTALL_UCRT_LIBRARIES TRUE)
        include(InstallRequiredSystemLibraries)
    endif()

    set(CPACK_GENERATOR "NSIS")
    set(CPACK_NSIS_PACKAGE_NAME "Fluxeem Driver ${PROJECT_VERSION}")
    set(CPACK_NSIS_DISPLAY_NAME "Fluxeem Driver ${PROJECT_VERSION}")
    set(CPACK_NSIS_REQUESTEXECUTIONLEVEL "admin")
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    set(CPACK_NSIS_MODIFY_PATH ON)
    set(CPACK_NSIS_HELP_LINK "https://www.fluxeem.com")
    set(CPACK_NSIS_URL_INFO_ABOUT "https://www.fluxeem.com")
    set(CPACK_NSIS_CONTACT "info@fluxeem.com")
    set(CPACK_NSIS_MANIFEST_DPI_AWARE ON)
    set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "${PROJECT_NAME}-${PROJECT_VERSION}")
    set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS
        [[  CreateDirectory '$WINDIR\Temp\FluxeemInstall'
  FileOpen $0 '$WINDIR\Temp\FluxeemInstall\FluxeemSetupLog.log' a
  FileWrite $0 '[NSIS] Install started$\r$\n'
  FileWrite $0 '[NSIS] InstDir: $INSTDIR$\r$\n'
  FileClose $0
  ExecWait 'cmd.exe /C ""$INSTDIR\installer\install_driver.bat""' $0
  FileOpen $1 '$WINDIR\Temp\FluxeemInstall\FluxeemSetupLog.log' a
  FileWrite $1 '[NSIS] install_driver.bat exit code: $0$\r$\n'
  FileWrite $1 '[NSIS] Install completed$\r$\n'
  FileClose $1
  SetFileAttributes '$WINDIR\Temp\FluxeemInstall\FluxeemSetupLog.log' HIDDEN
  RMDir /r '$INSTDIR\installer']]
    )
    set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS
        [[  CreateDirectory '$WINDIR\Temp\FluxeemInstall'
  FileOpen $0 '$WINDIR\Temp\FluxeemInstall\FluxeemSetupLog.log' a
  FileWrite $0 '[NSIS] Uninstall started$\r$\n'
  FileWrite $0 '[NSIS] Uninstall completed$\r$\n'
  FileClose $0
  SetFileAttributes '$WINDIR\Temp\FluxeemInstall\FluxeemSetupLog.log' HIDDEN]]
    )

    set(_fluxeem_nsis_create_links "")
    if(TARGET camera_control_ui)
        string(APPEND _fluxeem_nsis_create_links
            "  CreateShortCut '$SMPROGRAMS\\$STARTMENU_FOLDER\\Camera Control UI.lnk' '$INSTDIR\\${INSTALL_BINARY_DIR}\\camera_control_ui.exe'\n"
        )
    endif()
    if(TARGET fluxeem_live_viewer)
        string(APPEND _fluxeem_nsis_create_links
            "  CreateShortCut '$SMPROGRAMS\\$STARTMENU_FOLDER\\Fluxeem Live Viewer.lnk' '$INSTDIR\\${INSTALL_BINARY_DIR}\\fluxeem_live_viewer.exe'\n"
        )
    endif()
    if(_fluxeem_nsis_create_links)
        set(CPACK_NSIS_CREATE_ICONS_EXTRA
            "  CreateDirectory '$SMPROGRAMS\\$STARTMENU_FOLDER'\n${_fluxeem_nsis_create_links}"
        )
        set(CPACK_NSIS_DELETE_ICONS_EXTRA
            "  Delete '$SMPROGRAMS\\$STARTMENU_FOLDER\\Camera Control UI.lnk'\n  Delete '$SMPROGRAMS\\$STARTMENU_FOLDER\\Fluxeem Live Viewer.lnk'\n  RMDir '$SMPROGRAMS\\$STARTMENU_FOLDER'"
        )
    endif()
elseif(UNIX AND NOT APPLE)
    list(APPEND CPACK_GENERATOR "DEB")

    # Use a standard install prefix for Debian packages.
    set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")

    set(CPACK_DEBIAN_PACKAGE_NAME "fluxeem-driver")
    set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Fluxeem <info@fluxeem.com>")
    set(CPACK_DEBIAN_PACKAGE_SECTION "libs")
    set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
    set(CPACK_DEBIAN_PACKAGE_DESCRIPTION
        "Fluxeem Driver - Event Camera Driver and SDK.\n"
        " Provides shared libraries, public headers, and sample programs.")

    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS OFF)
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libusb-1.0-0 (>= 1.0.23)")

    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/installer/ubuntu/debian/postinst.in")
        configure_file(
            "${CMAKE_CURRENT_SOURCE_DIR}/installer/ubuntu/debian/postinst.in"
            "${CMAKE_BINARY_DIR}/postinst"
            @ONLY
        )
        execute_process(
            COMMAND chmod 755 "${CMAKE_BINARY_DIR}/postinst"
            RESULT_VARIABLE _fluxeem_postinst_chmod_result
            ERROR_VARIABLE _fluxeem_postinst_chmod_error
        )
        if(NOT _fluxeem_postinst_chmod_result EQUAL 0)
            message(FATAL_ERROR "Failed to set executable permission for postinst: ${_fluxeem_postinst_chmod_error}")
        endif()
        set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_BINARY_DIR}/postinst")
    else()
        message(WARNING "DEB postinst template not found: installer/ubuntu/debian/postinst.in")
    endif()
endif()

set(CPACK_PACKAGE_FILE_NAME
    "${PROJECT_NAME}-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")

include(CPack)

#[=======================================================================[.rst:
 LibUSB
 --------


 Find the libusb includes and library.


 IMPORTED Targets
 ^^^^^^^^^^^^^^^^


 This module defines :prop_tgt:`IMPORTED` target ``libusb-1.0``, if
 libusb-1.0 has been found.


 Result Variables
 ^^^^^^^^^^^^^^^^


 This module defines the following variables:


 ::


   LIBUSB_INCLUDE_DIR    - where to find libusb.h
   LIBUSB_LIBRARIES      - List of libraries when using libusb.
   LIBUSB_FOUND          - True if libusb found.


#]=======================================================================]



# Clear the cache
unset (LIBUSB_INCLUDE_DIR CACHE)
unset (LIBUSB_LIBRARIES CACHE)
unset (LIBUSB_RUNTIME_LIBRARY CACHE)


if(MSVC)
  # Typically on MSVC platforms there is no PkgConfig, so we rely on some defaults.
  # When using VCPKG for example, it will fix the path searching automatically.

  # 优先使用 VCPKG_ROOT 环境变量
  if(DEFINED ENV{VCPKG_ROOT})
    set(VCPKG_INSTALLED_DIR "$ENV{VCPKG_ROOT}/installed/x64-windows")
    message(STATUS "Using VCPKG_ROOT: $ENV{VCPKG_ROOT}")
  else()
    set(VCPKG_INSTALLED_DIR "C:/Tools/vcpkg/installed/x64-windows")
    message(WARNING "VCPKG_ROOT environment variable not set, using default path: ${VCPKG_INSTALLED_DIR}")
  endif()

  # 查找头文件 - 首先在 vcpkg 路径中查找
  find_path(LIBUSB_INCLUDE_DIR
      NAMES libusb.h
      PATHS "${VCPKG_INSTALLED_DIR}/include/libusb-1.0"
      PATH_SUFFIXES libusb-1.0
  )

  # 初始化 LIBUSB_LIBRARY 变量
  set(LIBUSB_LIBRARY "")

  # 查找 libusb 库文件（支持 Debug 和 Release）
  # 首先在 vcpkg 路径中查找（允许 CMake 在其他路径中回退搜索）
  find_library(LIBUSB_LIBRARY_RELEASE
      NAMES libusb-1.0.lib
      PATHS "${VCPKG_INSTALLED_DIR}/lib"
  )
  find_library(LIBUSB_LIBRARY_DEBUG
      NAMES libusb-1.0.lib
      PATHS "${VCPKG_INSTALLED_DIR}/debug/lib"
  )

    find_file(LIBUSB_RUNTIME_LIBRARY_RELEASE
      NAMES libusb-1.0.dll
      PATHS "${VCPKG_INSTALLED_DIR}/bin"
    )
    find_file(LIBUSB_RUNTIME_LIBRARY_DEBUG
      NAMES libusb-1.0.dll
      PATHS "${VCPKG_INSTALLED_DIR}/debug/bin"
    )

  # 使用适当的库（优先 Release）
  if(LIBUSB_LIBRARY_RELEASE AND LIBUSB_LIBRARY_DEBUG)
    set(LIBUSB_LIBRARY optimized ${LIBUSB_LIBRARY_RELEASE} debug ${LIBUSB_LIBRARY_DEBUG})
  elseif(LIBUSB_LIBRARY_RELEASE)
    set(LIBUSB_LIBRARY ${LIBUSB_LIBRARY_RELEASE})
  elseif(LIBUSB_LIBRARY_DEBUG)
    set(LIBUSB_LIBRARY ${LIBUSB_LIBRARY_DEBUG})
  endif()

  if(LIBUSB_RUNTIME_LIBRARY_RELEASE)
    set(LIBUSB_RUNTIME_LIBRARY ${LIBUSB_RUNTIME_LIBRARY_RELEASE})
  elseif(LIBUSB_RUNTIME_LIBRARY_DEBUG)
    set(LIBUSB_RUNTIME_LIBRARY ${LIBUSB_RUNTIME_LIBRARY_DEBUG})
  endif()

  # 打印查找结果
  if(LIBUSB_LIBRARY AND LIBUSB_INCLUDE_DIR)
    message(STATUS "Found libusb library: ${LIBUSB_LIBRARY}")
    message(STATUS "Found libusb include: ${LIBUSB_INCLUDE_DIR}")
  else()
    if(NOT LIBUSB_INCLUDE_DIR)
      message(WARNING "Could not find libusb-1.0 header file (libusb.h)")
    endif()
    if(NOT LIBUSB_LIBRARY)
      message(WARNING "Could not find libusb-1.0 library (libusb-1.0.lib)")
    endif()
    message(FATAL_ERROR "\nlibusb-1.0 not found!\n"
      "Please ensure:\\n"
      "  1. vcpkg is installed and libusb is installed:\\n"
      "       vcpkg install libusb:x64-windows\\n"
      "  2. VCPKG_ROOT environment variable is set to your vcpkg path:\\n"
      "       [Environment]::SetEnvironmentVariable('VCPKG_ROOT', 'C:\\\\path\\\\to\\\\vcpkg', 'User')\\n"
      "       (Restart VSCode after setting environment variable)")
  endif()
else()
    # use pkg-config to get the directories and then use these values
    # in the FIND_PATH() and FIND_LIBRARY() calls


    find_package(PkgConfig)
    pkg_check_modules(PC_LIBUSB libusb-1.0)


    find_path(LIBUSB_INCLUDE_DIR
        NAMES libusb.h
        PATHS ${PC_LIBUSB_INCLUDEDIR} ${PC_LIBUSB_INCLUDE_DIRS}
    )


    find_library(LIBUSB_LIBRARY NAMES usb-1.0
        PATHS ${PC_LIBUSB_LIBDIR} ${PC_LIBUSB_LIBRARY_DIRS}
    )

endif(MSVC)


include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibUSB DEFAULT_MSG LIBUSB_INCLUDE_DIR LIBUSB_LIBRARY)


if (LIBUSB_FOUND AND NOT TARGET libusb-1.0)
    add_library(libusb-1.0 UNKNOWN IMPORTED)

    # 检查是否有 Release/Debug 双版本
    if(LIBUSB_LIBRARY_RELEASE AND LIBUSB_LIBRARY_DEBUG)
        # 分别设置 Release 和 Debug 版本的库
      # 对 MSVC，.lib 是导入库，真正运行时文件是 .dll。
        set_target_properties(libusb-1.0 PROPERTIES
        IMPORTED_IMPLIB_RELEASE    "${LIBUSB_LIBRARY_RELEASE}"
        IMPORTED_IMPLIB_DEBUG      "${LIBUSB_LIBRARY_DEBUG}"
        IMPORTED_LOCATION          "${LIBUSB_RUNTIME_LIBRARY_RELEASE}"
        IMPORTED_LOCATION_RELEASE  "${LIBUSB_RUNTIME_LIBRARY_RELEASE}"
        IMPORTED_LOCATION_DEBUG    "${LIBUSB_RUNTIME_LIBRARY_DEBUG}"
            INTERFACE_INCLUDE_DIRECTORIES "${LIBUSB_INCLUDE_DIR}")
    else()
        # 单一版本库
        set_target_properties(libusb-1.0 PROPERTIES
        IMPORTED_IMPLIB "${LIBUSB_LIBRARY}"
        IMPORTED_LOCATION "${LIBUSB_RUNTIME_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${LIBUSB_INCLUDE_DIR}")
    endif()
endif (LIBUSB_FOUND AND NOT TARGET libusb-1.0)


mark_as_advanced(
    LIBUSB_INCLUDE_DIR
    LIBUSB_LIBRARY
)


set (LIBUSB_LIBRARIES ${LIBUSB_LIBRARY})
set (LIBUSB_RUNTIME_LIBRARIES ${LIBUSB_RUNTIME_LIBRARY})

# Keep vcpkg paths visible for CMake module-mode finders.
# This is required by some transitive dependencies (e.g. OpenCV -> TIFF).
if(MSVC AND DEFINED ENV{VCPKG_ROOT})
    cmake_path(CONVERT "$ENV{VCPKG_ROOT}" TO_CMAKE_PATH_LIST FLUXEEM_VCPKG_ROOT)
    set(FLUXEEM_VCPKG_INSTALLED_DIR "${FLUXEEM_VCPKG_ROOT}/installed/x64-windows")

    if(EXISTS "${FLUXEEM_VCPKG_INSTALLED_DIR}")
        list(PREPEND CMAKE_PREFIX_PATH
            "${FLUXEEM_VCPKG_INSTALLED_DIR}"
            "${FLUXEEM_VCPKG_INSTALLED_DIR}/share"
        )
        list(PREPEND CMAKE_INCLUDE_PATH "${FLUXEEM_VCPKG_INSTALLED_DIR}/include")
        list(PREPEND CMAKE_LIBRARY_PATH
            "${FLUXEEM_VCPKG_INSTALLED_DIR}/lib"
            "${FLUXEEM_VCPKG_INSTALLED_DIR}/debug/lib"
        )

        # Help FindTIFF when OpenCV uses find_dependency(TIFF) in module mode.
        if((NOT TIFF_INCLUDE_DIR) AND EXISTS "${FLUXEEM_VCPKG_INSTALLED_DIR}/include/tiff.h")
            set(TIFF_INCLUDE_DIR "${FLUXEEM_VCPKG_INSTALLED_DIR}/include" CACHE PATH "TIFF include directory" FORCE)
        endif()

        if(NOT TIFF_LIBRARY)
            if(EXISTS "${FLUXEEM_VCPKG_INSTALLED_DIR}/lib/tiff.lib")
                set(TIFF_LIBRARY "${FLUXEEM_VCPKG_INSTALLED_DIR}/lib/tiff.lib" CACHE FILEPATH "TIFF library" FORCE)
            elseif(EXISTS "${FLUXEEM_VCPKG_INSTALLED_DIR}/debug/lib/tiff.lib")
                set(TIFF_LIBRARY "${FLUXEEM_VCPKG_INSTALLED_DIR}/debug/lib/tiff.lib" CACHE FILEPATH "TIFF library" FORCE)
            endif()
        endif()
    endif()
endif()

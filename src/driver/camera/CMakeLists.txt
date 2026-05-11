# SPDX-FileCopyrightText: 2024-present Fluxeem Technologies Co., Ltd.
# SPDX-License-Identifier: Apache-2.0
# CMakeLists.txt for src/driver/camera directory

target_sources(${LibName} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/base/i_camera.cpp
    ${CMAKE_CURRENT_LIST_DIR}/base/ev_camera_service.cpp
    ${CMAKE_CURRENT_LIST_DIR}/base/event_camera.cpp
    ${CMAKE_CURRENT_LIST_DIR}/base/imx636_camera.cpp
    ${CMAKE_CURRENT_LIST_DIR}/device/prophesee_evk4.cpp
    ${CMAKE_CURRENT_LIST_DIR}/device/prophesee_evk5.cpp
    ${CMAKE_CURRENT_LIST_DIR}/device/dvs_lume.cpp
    ${CMAKE_CURRENT_LIST_DIR}/device/prophesee_rdk3.cpp
    ${CMAKE_CURRENT_LIST_DIR}/device/fluxeem_apex_vision_s1.cpp
)

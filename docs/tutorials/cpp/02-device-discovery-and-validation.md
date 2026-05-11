# 设备发现与环境验证 {#camera_discovery}

@brief 使用 SDK 示例程序确认设备连接和运行环境

本教程介绍如何基于 SDK 已提供的程序，验证相机连接状态以及 SDK 运行环境是否正确。

## 教程目标

- 确认相机已经被 SDK 正常识别
- 理解设备发现的基本过程
- 建立对 `EvCameraService` 的初步认识

## 1. 验证思路

对于 SDK 二次开发而言，第一步不是自己编写代码，而是先确认交付包中的现成程序能够正常工作。

推荐验证链路如下：

1. 运行Live viewer，确认相机可被识别和打开
2. 在示例可正常运行的基础上，再进行业务程序开发

这种方式可以将“安装问题”“运行时依赖问题”“设备连接问题”和“用户代码问题”区分开，减少排查成本。

## 2. 使用Live viewer验证设备

运行 SDK 的 `fluxeem_live_viewer`：

```text
<SDK_INSTALL_DIR>/bin/fluxeem_live_viewer
```

若设备连接正常，程序通常会完成以下流程：

1. 创建设备管理对象
2. 枚举当前可用相机
3. 选择目标相机并打开
4. 启动数据流
5. 将事件数据绘制到显示窗口

如果界面正常打开并显示实时事件图像，就说明设备发现链路已经打通。

## 3. 设备发现接口简介

SDK 中用于相机发现和打开的核心入口是 `fluxeem::EvCameraService`。

典型使用流程如下：

```cpp
#include "driver/camera/ev_camera_service.hpp"
#include <iostream>

int main()
{
    fluxeem::EvCameraService service;

    auto cameras = service.listCameras();
    if (cameras.empty()) {
        std::cout << "No camera found." << std::endl;
        return 1;
    }

    auto camera = service.open(cameras.front().serial);
    if (!camera) {
        std::cout << "Open camera failed." << std::endl;
        return 2;
    }

    std::cout << "Camera opened: " << cameras.front().serial << std::endl;
    return 0;
}
```

在这个流程中：

- `listCameras()` 用于获取当前可用设备列表
- `open(serial)` 用于按序列号打开指定相机

这也是大多数 SDK 二次开发程序的起点。

## 4. 使用 CMake 编译设备发现示例

如果需要基于已安装 SDK 自己编译一个设备发现示例，可以使用如下最小 CMake 工程：

```cmake
cmake_minimum_required(VERSION 3.10)
project(camera_discovery_sample)

set(CMAKE_CXX_STANDARD 20)

find_package(fluxeem_driver CONFIG REQUIRED)

add_executable(camera_discovery_sample main.cpp)
target_link_libraries(camera_discovery_sample PRIVATE fluxeem::fluxeem_driver)
```

对应的 `main.cpp` 可以直接使用本页中的最小发现示例。

配置和构建示例：

```text
cmake -S . -B build -DCMAKE_PREFIX_PATH=<SDK_INSTALL_DIR>
cmake --build build
```

如果 SDK 已安装到系统默认搜索路径，也可以省略 `CMAKE_PREFIX_PATH`。

## 5. 验证通过的判断标准

如果满足以下条件，说明环境验证基本完成：

- 示例程序可正常启动
- 可以列出至少一个相机
- 可以成功打开目标相机
- 实时显示程序能够稳定获取事件数据

## 6. 连接异常时的检查建议

当示例程序无法发现相机时，建议按以下顺序检查：

- 相机是否连接在 USB 3.0 接口
- 是否更换过数据线或转接器
- 相机是否被其他程序占用
- Linux 下是否具有访问设备节点的权限
- 是否使用了与当前系统匹配的 SDK 包

## 下一步

- @ref live_viewer "学习Live viewer和基础相机接口"

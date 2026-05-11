# Live viewer与基础接口 {#live_viewer}

@brief 通过 fluxeem_live_viewer 熟悉相机基础 API

本教程基于 SDK 自带的 `fluxeem_live_viewer` 示例程序，介绍事件相机应用中最常见的一组基础接口，帮助用户建立对相机打开、启动、事件接收和显示流程的基本认识。

## 教程目标

- 理解 `fluxeem_live_viewer` 的程序结构
- 熟悉相机打开与启动流程
- 了解事件回调接口的基本用法
- 理解事件数据显示的最小闭环

## 1. 程序定位

`fluxeem_live_viewer` 是 SDK 中最适合作为入门参考的示例程序之一。

它完成了一个典型事件相机应用的最小功能闭环：

1. 枚举可用相机
2. 打开目标相机
3. 获取分辨率信息
4. 注册事件回调
5. 启动相机
6. 将事件流转换为可视化图像
7. 在界面中持续显示

对于大多数二次开发项目，这些步骤都是上层应用的基础组成部分。

## 2. 设备打开流程

示例程序首先通过 `EvCameraService` 发现并打开设备：

```cpp
fluxeem::EvCameraService camera_manager;
const auto descs = camera_manager.listCameras();
auto cam = camera_manager.open(descs.front().serial);
```

这一步完成了两件事：

- 获取当前可用设备列表
- 返回一个 `ICamera` 接口对象，供后续控制与数据接收使用

## 3. 获取设备信息

相机打开后，通常会先读取基础信息，例如分辨率：

```cpp
const uint16_t width = cam->getWidth();
const uint16_t height = cam->getHeight();
```

这些信息常用于：

- 创建显示缓冲区
- 分配算法输入缓存
- 初始化图像或事件统计模块

## 4. 注册事件回调

`fluxeem_live_viewer` 使用事件批次回调来接收数据：

```cpp
const uint32_t cb_id = cam->registerEventBatchCallback(
    [](fluxeem::EventIterator_t begin, fluxeem::EventIterator_t end)
    {
        // process events
    });
```

这里的关键点是：

- 回调拿到的是一个事件区间 `[begin, end)`
- 用户可遍历区间内的所有事件
- 每个事件通常包含坐标、时间戳和极性

这是 SDK 二次开发中最常见的数据接入方式之一。

## 5. 使用 CMake 编译 `fluxeem_live_viewer`

`fluxeem_live_viewer` 依赖已安装的 Fluxeem Driver SDK 和 OpenCV。基于 SDK 的独立 CMake 工程可以写成：

```cmake
cmake_minimum_required(VERSION 3.10)
project(fluxeem_live_viewer_demo)

set(CMAKE_CXX_STANDARD 20)

find_package(fluxeem_driver CONFIG REQUIRED)
find_package(OpenCV REQUIRED)

add_executable(fluxeem_live_viewer_demo main.cpp)
target_link_libraries(fluxeem_live_viewer_demo
    PRIVATE
        fluxeem::fluxeem_driver
        ${OpenCV_LIBS}
)

target_include_directories(fluxeem_live_viewer_demo PRIVATE ${OpenCV_INCLUDE_DIRS})
```

构建时请确保：

- `fluxeem_driver` 可以被 `find_package` 找到
- OpenCV 可以被 `find_package(OpenCV REQUIRED)` 找到

配置和构建示例：

```text
cmake -S . -B build -DCMAKE_PREFIX_PATH=<SDK_INSTALL_DIR>
cmake --build build
```

## 6. 启动与停止相机

完成回调注册后，程序启动相机开始采集：

```cpp
if (cam->start() != 0) {
    return 1;
}
```

退出前停止相机：

```cpp
cam->stop();
```

在上层应用中，`start()` 和 `stop()` 一般分别对应：

- 开始实时采集
- 停止采集并释放运行态资源

## 7. 事件显示逻辑

实时显示器并不是直接显示“帧图像”，而是把一个时间窗内的事件累积成一张图：

- 正极性事件通常显示为绿色
- 负极性事件通常显示为红色
- 达到时间窗后更新显示缓存

这种方式有助于初学者快速理解事件流的时空分布，也常用于设备调试、链路验证和算法联调。

## 8. 建议重点掌握的接口

阅读或基于 `fluxeem_live_viewer` 进行二次开发时，建议优先熟悉以下接口：

- `EvCameraService::listCameras()`
- `EvCameraService::open()`
- `ICamera::getWidth()`
- `ICamera::getHeight()`
- `ICamera::registerEventBatchCallback()`
- `ICamera::start()`
- `ICamera::stop()`

理解这些接口后，用户通常已经能够完成一个最基础的采集与显示程序。

## 9. 二次开发建议

基于该示例继续开发时，建议优先做以下扩展：

- 将显示逻辑替换为自己的处理逻辑
- 在回调中增加事件统计或算法输入转换
- 增加相机序列号选择
- 增加启动、停止和录制控制

## 下一步

- @ref camera_config "学习 tool 接口与参数配置"
- @ref recording "学习 file reader 与 RAW 数据处理"

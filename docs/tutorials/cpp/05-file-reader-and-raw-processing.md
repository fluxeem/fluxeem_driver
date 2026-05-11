# File Reader 与 RAW 数据处理 {#recording}

@brief 使用 SDK 中的 file reader 读取并处理保存好的 RAW 数据

本教程介绍如何使用 SDK 中的 file reader 接口，对已经保存好的 RAW 数据进行读取、遍历和处理。该流程主要用于离线分析、算法回放和问题复现。

## 教程目标

- 理解 file reader 的使用场景
- 学会打开并加载 RAW 文件
- 学会按批次读取事件数据
- 建立离线处理程序的基本开发思路

## 1. 使用场景

RAW 文件读取主要适用于以下场景：

- 采集后离线分析
- 算法复现与调试
- 问题数据回放
- 不连接真实相机时进行数据链路验证

对于二次开发来说，file reader 可以让上层处理逻辑同时支持：

- 在线实时数据
- 离线录制数据

这对于开发和测试都非常有价值。

## 2. 创建读取器并加载文件

SDK 中通常通过 `EvFileReader` 创建文件读取对象：

```cpp
#include "driver/file_reader/ev_file_reader.h"

auto reader = fluxeem::EvFileReader::createFileReader("sample.raw");
if (!reader) {
    return 1;
}

if (!reader->open()) {
    return 2;
}
```

完成 `open()` 后，程序即可开始读取文件中的事件数据。

## 3. 读取文件基本信息

在进入正式处理前，建议先获取文件基本信息，例如：

- 事件总数
- 起始时间戳
- 结束时间戳
- 分辨率

这些信息有助于：

- 估算处理规模
- 初始化显示或缓存
- 做进度显示
- 做时间范围控制

## 4. 按批次读取数据

典型的离线处理方式是按批次读取事件：

```cpp
while (!reader->isEndReached()) {
    auto events = reader->getNEvents(10000);
    if (!events || events->empty()) {
        break;
    }

    for (const auto& ev : *events) {
        // process event
    }
}
```

这种方式适合：

- 做统计分析
- 做事件过滤
- 转换成上层算法输入
- 转存为其他格式

## 5. 使用 CMake 编译 `raw_file_reader_sample`

`raw_file_reader_sample` 依赖已安装的 Fluxeem Driver SDK 和 OpenCV。独立编译时可使用如下 CMake 工程：

```cmake
cmake_minimum_required(VERSION 3.10)
project(raw_file_reader_demo)

set(CMAKE_CXX_STANDARD 20)

find_package(fluxeem_driver CONFIG REQUIRED)
find_package(OpenCV REQUIRED)

add_executable(raw_file_reader_demo main.cpp)
target_link_libraries(raw_file_reader_demo
    PRIVATE
        fluxeem::fluxeem_driver
        ${OpenCV_LIBS}
)

target_include_directories(raw_file_reader_demo PRIVATE ${OpenCV_INCLUDE_DIRS})
```

配置和构建示例：

```text
cmake -S . -B build -DCMAKE_PREFIX_PATH=<SDK_INSTALL_DIR>
cmake --build build
```

## 6. 示例程序说明

SDK 中的 `raw_file_reader_sample` 是理解 file reader 的最好入口之一。

它展示了以下典型能力：

- 打开 RAW 文件
- 按时间或事件批次进行回放
- 进行事件累积显示
- 支持播放、暂停、跳转和变速

对于用户来说，这个示例说明了 file reader 不只是“读文件”，还可以用于构建完整的离线回放工具。

## 7. 二次开发建议

在业务程序中使用 file reader 时，建议采用以下结构：

1. 加载目标 RAW 文件
2. 读取文件基本信息
3. 以固定批次或固定时间窗读取事件
4. 将事件送入自己的处理模块
5. 根据需要输出统计、图像或分析结果

如果你的在线处理逻辑本身已经基于事件批次工作，那么通常只需要把数据源从“相机”切换为“文件读取器”，就可以复用大量处理代码。

## 8. 适合重点关注的接口

使用 file reader 时，建议重点了解以下能力：

- 创建读取器
- 加载文件
- 判断是否到达结尾
- 按事件数量读取
- 按时间范围读取
- 跳转到指定位置或时间

## 下一步

- @ref hardware_sync "学习 HAL Sync 与硬件同步流程"

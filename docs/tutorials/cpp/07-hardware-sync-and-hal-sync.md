# 硬件同步与 HAL Sync {#hardware_sync}

@brief 使用 HAL Sync 接口完成事件相机硬件同步演示

本教程介绍如何使用 SDK 中的硬件同步能力，对事件相机进行主从同步配置，并结合示例程序理解硬件接线和程序启动流程。

## 教程目标

- 理解硬件同步的基本用途
- 了解主从相机的连接关系
- 学会使用示例程序验证同步链路
- 建立基于 HAL Sync 的二次开发思路

## 1. 使用场景

硬件同步主要用于以下场景：

- 多相机联合采集
- 多设备时间对齐
- 外部时钟或外部触发协同
- 对时间一致性要求较高的实验或系统

在这些场景中，硬件连接方式和软件配置流程都很重要，缺少其中任意一项都可能导致同步失败。

## 2. 硬件连接说明

进行同步演示前，请先确认：

- 所有相机供电和 USB 数据连接正常

- 主相机与从相机之间的同步线连接正确
 

![引脚定义](..\img\pin_define.png)



​	对于官方配件线缆，可参考以下管脚与线色定义：


 | 管脚 | 信号类型   | 说明                           | 线缆颜色（官方配件） |
 | ---- | ---------- | ------------------------------ | -------------------- |
 | 1    | `VCC`      | 电源输入，11-25V               | 红色                 |
 | 2    | `OPTO_IN`  | 数字输入，光耦隔离触发输入     | 黄色                 |
 | 3    | `SYNC`     | 数字双向同步信号，主从模式时钟 | 蓝色                 |
| 4    | 预留       | 未定义                         | 绿色                 |
 | 5    | `OPTO_GND` | 数字地，光耦隔离地             | 白色                 |
| 6    | `GND`      | 电源地 / 同步信号地            | 黑色                 |

- 接线方向与系统定义一致

- 在接线完成前不要随意切换同步模式

通常情况下：

- `MASTER` 负责输出同步信号
- `SLAVE` 负责接收同步信号
- `STANDALONE` 表示单机独立运行



在进行主从同步接线时，建议特别关注以下几点：

- `SYNC` 线用于主从同步时钟或同步信号传输
- `GND` 必须正确连接，作为电源与同步信号参考地
- `OPTO_IN` 和 `OPTO_GND` 主要用于光耦隔离触发输入场景
- `VCC` 接线前必须确认供电范围满足 11-25V 要求
- 预留脚位不要接入未确认用途的外部信号

## 3. 示例程序说明

SDK 中的 `camera_hardware_sync_sample` 演示了基本同步模式切换和启动流程。

其基本调用方式为：

```text
camera_hardware_sync_sample <mode> <serial_number>
```

其中：

- `mode` 可选 `MASTER`、`SLAVE`、`STANDALONE`
- `serial_number` 为目标相机序列号

示例：

```text
camera_hardware_sync_sample MASTER 00000000
camera_hardware_sync_sample SLAVE 11111111
```

## 4. 使用 CMake 编译 `camera_hardware_sync_sample`

该示例依赖已安装的 Fluxeem Driver SDK 和 OpenCV。独立编译时可参考如下 CMake 写法：

```cmake
cmake_minimum_required(VERSION 3.10)
project(camera_hardware_sync_demo)

set(CMAKE_CXX_STANDARD 20)

find_package(fluxeem_driver CONFIG REQUIRED)
find_package(OpenCV REQUIRED)

add_executable(camera_hardware_sync_demo main.cpp)
target_link_libraries(camera_hardware_sync_demo
    PRIVATE
        fluxeem::fluxeem_driver
        ${OpenCV_LIBS}
)

target_include_directories(camera_hardware_sync_demo PRIVATE ${OpenCV_INCLUDE_DIRS})
```

配置和构建示例：

```text
cmake -S . -B build -DCMAKE_PREFIX_PATH=<SDK_INSTALL_DIR>
cmake --build build
```

## 5. 程序启动流程

该示例的典型流程如下：

1. 枚举相机并确认目标序列号存在
2. 打开目标相机
3. 获取 `TOOL_SYNC`
4. 设置同步模式
5. 注册事件回调
6. 启动相机
7. 显示事件数据并观察同步效果
8. 退出前恢复为 `STANDALONE`

这也是实际项目中使用同步功能的基础思路。

## 6. Sync tool 的使用方式

示例程序中通过如下方式访问同步工具：

```cpp
auto sync_tool = camera->getTool(fluxeem::ToolType::TOOL_SYNC);
if (sync_tool) {
    sync_tool->setParam("mode", "MASTER");
}
```

这说明同步功能在上层仍然遵循统一的 tool 使用模式：

- 获取工具
- 写入参数
- 启动采集

## 7. 验证建议

在做同步演示时，建议按如下步骤进行：

1. 先分别验证每台相机在 `STANDALONE` 模式下都能正常工作
2. 确认同步接线正确
3. 先启动 `MASTER`，再启动 `SLAVE`
4. 观察事件输出或业务层时间一致性表现
5. 退出后恢复独立模式

## 8. 二次开发建议

如果你需要在自己的程序中加入同步能力，建议：

- 把同步模式设置流程与普通采集流程解耦
- 在进入同步模式前做设备状态检查
- 对同步模式切换和恢复过程加日志
- 把硬件连接状态检查写进操作文档或界面提示中

## 下一步

- @ref tutorial_cpp_index "返回 C++ 教程目录"

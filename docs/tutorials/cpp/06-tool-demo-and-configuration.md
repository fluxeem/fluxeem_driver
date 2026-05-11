# Tool 示例程序说明 {#tool_demo}

@brief 通过 tool 示例理解相机参数配置流程

本教程重点介绍如何把 tool 配置能力用于二次开发程序中。对于客户侧开发而言，目标是掌握“如何配置相机”，而不是理解驱动内部如何实现这些配置。

## 教程目标

- 理解 tool 配置程序的基本结构
- 知道如何在程序中组织参数设置流程
- 了解典型的相机配置场景

## 1. 为什么需要单独关注 tool 示例

相机接入成功后，很多实际项目很快就会进入调参阶段，例如：

- 只保留局部区域事件
- 调整事件灵敏度
- 启用外部触发
- 控制事件速率
- 配置同步模式

因此，tool 部分往往是 SDK 二次开发中最容易从“跑通示例”过渡到“做自己的业务程序”的环节。

## 2. 推荐参考程序

当前仓库中，`camera_control_ui` 提供了一个比较直观的工具配置参考。

它说明了两件事：

- 上层程序可以枚举并组织多个 tool
- 不同 tool 可以组合成一个完整的设备配置界面或配置流程

即使你的最终程序不是图形界面，也可以参考它的整体结构来设计自己的参数管理模块。

## 3. 使用 CMake 编译 tool 示例程序

如果需要编译类似 `camera_control_ui` 的 tool 示例程序，通常需要链接已安装的 Fluxeem Driver SDK 和 OpenCV：

```cmake
cmake_minimum_required(VERSION 3.10)
project(camera_control_ui_demo)

set(CMAKE_CXX_STANDARD 20)

find_package(fluxeem_driver CONFIG REQUIRED)
find_package(OpenCV REQUIRED)

add_executable(camera_control_ui_demo main.cpp)
target_link_libraries(camera_control_ui_demo
    PRIVATE
        fluxeem::fluxeem_driver
        ${OpenCV_LIBS}
)

target_include_directories(camera_control_ui_demo PRIVATE ${OpenCV_INCLUDE_DIRS})
```

配置和构建示例：

```text
cmake -S . -B build -DCMAKE_PREFIX_PATH=<SDK_INSTALL_DIR>
cmake --build build
```

## 4. 二次开发时建议的配置流程

建议将 tool 配置流程组织为以下步骤：

1. 打开相机
2. 查询当前设备支持的 tool
3. 查询每个 tool 的参数列表
4. 根据业务需求写入参数
5. 回读参数确认配置成功
6. 在必要时保存配置记录

## 5. 典型应用方式

### 启动时加载默认配置

程序启动后自动对相机施加一组固定参数，适合量产或固定场景应用。

### 运行时动态调参

程序运行时允许用户修改 ROI、Bias 或过滤器参数，适合调试工具和实验系统。

### 配置模板切换

为不同场景准备多套配置模板，在运行时切换，适合演示系统或多工况设备。

## 6. 工程建议

在做自己的 tool 程序时，建议：

- 不要假设所有设备都支持完全相同的工具
- 不要把参数名称和范围硬编码到业务逻辑中
- 尽量先读能力描述，再生成配置项
- 对关键参数变更保留日志

## 下一步

- @ref recording "返回 file reader 教程"
- @ref hardware_sync "进入硬件同步教程"

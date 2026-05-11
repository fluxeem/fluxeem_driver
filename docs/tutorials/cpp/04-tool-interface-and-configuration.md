# Tool 接口与参数配置 {#camera_config}

@brief 使用 SDK 中的 tool 功能配置相机参数

本教程介绍如何使用 SDK 中的 tool 接口，对事件相机进行常见参数配置。教程重点是帮助用户理解如何在二次开发中访问和使用工具模块，而不是讨论底层驱动实现。

## 教程目标

- 理解 tool 接口的作用
- 学会获取相机支持的工具对象
- 了解常见参数配置流程
- 为后续业务程序中的设备调参建立参考

## 1. Tool 的作用

在 Fluxeem Driver SDK 中，tool 用于承载与设备配置相关的功能模块。对当前 IMX636/646 的实现而言，常见 tool 包括：ROI、Bias、TriggerIn、Sync、AntiFlicker、EventTrailFilter 和 EventRateControl。

上层程序通常不需要直接操作底层寄存器，而是通过统一的 tool 接口完成参数设置和读取。下表按工具逐项列出参数、最小值、最大值和调整效果。

### ROI

作用：设置事件保留区域或排除区域，减少无关事件。

约束条件：`x + width <= 1280`，`y + height <= 720`，且 `width > 0`、`height > 0`。

| 参数 | 类型 | 最小值 | 最大值 | 可选值/单位 | 调整效果（增大/减小） | 使用建议 |
| --- | --- | --- | --- | --- | --- | --- |
| `enable` | bool | `false` | `true` | 开/关 | `true` 启用 ROI 逻辑；`false` 关闭 ROI 逻辑 | 建议先完成坐标和尺寸设置，再置 `true` |
| `mode` | enum | N/A | N/A | `ROI` / `RONI` | `ROI` 保留窗口内事件；`RONI` 排除窗口内事件 | 目标区域增强用 `ROI`，固定干扰区抑制用 `RONI` |
| `x` | int | `0` | `1280` | pixel | 增大：窗口整体右移；减小：窗口整体左移 | 与 `width` 联合满足边界约束 |
| `y` | int | `0` | `720` | pixel | 增大：窗口整体下移；减小：窗口整体上移 | 与 `height` 联合满足边界约束 |
| `width` | int | `1`（生效时） | `1280` | pixel | 增大：覆盖横向范围增大；减小：覆盖横向范围减小 | 过小会丢失横向细节 |
| `height` | int | `1`（生效时） | `720` | pixel | 增大：覆盖纵向范围增大；减小：覆盖纵向范围减小 | 过小会丢失纵向细节 |

### Bias

作用：调节像素前端偏置，影响事件触发灵敏度、噪声水平和明暗变化响应。

说明：Bias 参数为相对硬件基线的 offset。`0` 表示基线，正负值表示在基线上下偏移。

| 参数 | 类型 | 最小值 | 最大值 | 单位 | 调整效果（增大/减小） | 主要影响 |
| --- | --- | --- | --- | --- | --- | --- |
| `bias_fo` | int | `-20` | `0` | `%` | 增大（趋近 0）：通常提高前端响应速度，噪声风险上升；减小：通常提高稳定性，但高速细节可能减少 | 响应速度与稳定性平衡 |
| `bias_hpf` | int | `0` | `120` | `%` | 增大：增强慢变化抑制，背景漂移事件减少；减小：保留更多低频变化，同时背景波动可能增多 | 低频光照变化抑制 |
| `bias_diff_on` | int | `-80` | `145` | `%` | 增大：通常提高 ON（变亮）事件触发强度；减小：ON 事件更保守 | 亮边缘与增亮瞬态响应 |
| `bias_diff` | int | `-25` | `23` | `%` | 增大：整体差分通道响应趋强；减小：整体响应趋稳 | 整体灵敏度细调 |
| `bias_diff_off` | int | `-30` | `200` | `%` | 增大：通常提高 OFF（变暗）事件触发强度；减小：OFF 事件更保守 | 暗边缘与变暗瞬态响应 |
| `bias_refr` | int | `-20` | `235` | `%` | 增大：通常增强重复触发抑制；减小：保留更多高频重复变化 | 重复事件抑制与细节保留 |

| 现象 | 优先调整参数 | 调整方向 |
| --- | --- | --- |
| 亮边缘漏检 | `bias_diff_on` | 先小步增大 |
| 暗边缘漏检 | `bias_diff_off` | 先小步增大 |
| 背景慢闪、慢漂移 | `bias_hpf` | 先小步增大 |
| 局部重复事件过多 | `bias_refr` | 先小步增大 |
| 噪声明显升高 | 最近改动的 Bias 参数 | 先回退一档 |

### TriggerIn

作用：控制是否接收外部触发输入信号。

| 参数 | 类型 | 最小值 | 最大值 | 可选值/单位 | 调整效果（增大/减小） | 使用建议 |
| --- | --- | --- | --- | --- | --- | --- |
| `enable` | bool | `false` | `true` | 开/关 | `true`：使能外部触发输入；`false`：禁用外部触发输入 | 仅在系统存在外部触发源时开启 |

### Sync

作用：配置设备时间基准模式，用于单机或多设备同步。

| 参数 | 类型 | 最小值 | 最大值 | 可选值/单位 | 调整效果（增大/减小） | 使用建议 |
| --- | --- | --- | --- | --- | --- | --- |
| `mode` | enum | N/A | N/A | `STANDALONE` / `MASTER` / `SLAVE` | `STANDALONE`：本机独立时基；`MASTER`：输出同步基准；`SLAVE`：接收外部同步基准 | 多机同步时通常 1 台 `MASTER`，其余 `SLAVE` |

### AntiFlicker

作用：抑制工频照明或周期光源造成的闪烁事件。

约束条件：`low_frequency <= high_frequency`。

| 参数 | 类型 | 最小值 | 最大值 | 可选值/单位 | 调整效果（增大/减小） | 使用建议 |
| --- | --- | --- | --- | --- | --- | --- |
| `enable` | bool | `false` | `true` | 开/关 | `true` 启用抗闪烁；`false` 关闭抗闪烁 | 仅在存在稳定闪烁干扰时开启 |
| `low_frequency` | int | `50` | `520` | hz | 增大：抑制频段下边界上移，低频抑制范围缩小；减小：下边界下移，低频抑制范围扩大 | 先定位闪烁主频，再设置下边界 |
| `high_frequency` | int | `50` | `520` | hz | 增大：抑制频段上边界上移，高频抑制范围扩大；减小：上边界下移，高频抑制范围缩小 | 与 `low_frequency` 成对调整 |
| `fliter_mode` | enum | N/A | N/A | `Band cut` / `Band pass` | `Band cut`：抑制目标频段；`Band pass`：仅保留目标频段 | 业务采集优先 `Band cut` |
| `duty_cycle` | float | `>0` | `100` | `%` | 增大：模型中高电平占比增大；减小：模型中高电平占比减小 | 仅当已知光源占空比时再精调 |
| `start_threshold` | int | `0` | `7` | count | 增大：进入滤波的判定更严格；减小：更容易进入滤波 | 启停抖动时优先增大 |
| `stop_threshold` | int | `0` | `7` | count | 增大：退出滤波判定门限提高（通常保持滤波时间更长）；减小：更容易退出滤波 | 与 `start_threshold` 联合调整 |

### EventTrailFilter

作用：对同一像素位置的同极性短时重复事件进行抑制（时间阈值判定）。

| 参数 | 类型 | 最小值 | 最大值 | 可选值/单位 | 调整效果（增大/减小） | 使用建议 |
| --- | --- | --- | --- | --- | --- | --- |
| `enable` | bool | `false` | `true` | 开/关 | `true` 启用短时去重滤波；`false` 关闭短时去重滤波 | 观察到同像素高频重复事件时开启 |
| `threshold` | int | `1` | `100` | ms | 增大：滤波时间窗口变大，抑制更强；减小：滤波更保守，细节保留更多 | 建议从中小值起步逐步上调 |
| `type` | enum | N/A | N/A | `TRAIL` / `STC_CUT_TRAIL` / `STC_KEEP_TRAIL` | `TRAIL`：标准短时去重；`STC_CUT_TRAIL`：更强短时抑制；`STC_KEEP_TRAIL`：折中模式 | 优先 `TRAIL`，不足再切换 STC 模式 |

### EventRateControl

作用：限制总事件率，避免链路和下游处理过载。

| 参数 | 类型 | 最小值 | 最大值 | 可选值/单位 | 调整效果（增大/减小） | 使用建议 |
| --- | --- | --- | --- | --- | --- | --- |
| `enable` | bool | `false` | `true` | 开/关 | `true` 启用事件率限制；`false` 不限速 | 带宽受限或算法过载时开启 |
| `max_event_rate` | int | `0` | `320` | MEv/s | 增大：通过事件更多、细节保留更充分、系统负载上升；减小：通过事件更少、负载下降、细节可能损失 | 先设高值确保效果，再逐步下调到稳定点 |

## 2. 获取 tool 对象

相机打开后，可以通过 `getTool()` 获取指定功能模块：

```cpp
auto roi_tool = camera->getTool(fluxeem::ToolType::TOOL_ROI);
auto bias_tool = camera->getTool(fluxeem::ToolType::TOOL_BIAS);
auto trigger_tool = camera->getTool(fluxeem::ToolType::TOOL_TRIGGER_IN);
```

如果某个工具在当前设备上不可用，返回值可能为空，因此建议先判空再继续调用。

## 3. 查询工具能力

在实际项目中，不同型号相机支持的工具和参数可能并不完全相同。推荐先读取工具信息，再决定如何配置：

```cpp
auto tool = camera->getTool(fluxeem::ToolType::TOOL_ROI);
if (tool) {
    auto info = tool->getToolInfo();
    for (const auto& name : info.parameter_names) {
        // inspect available parameters
    }
}
```

这种方式更适合做可移植的上层应用或设备调试工具。

## 4. 参数设置的基本模式

tool 的典型使用流程如下：

1. 获取工具对象
2. 确认工具支持的参数
3. 写入目标参数
4. 回读参数确认设置结果

示例：

```cpp
auto roi_tool = camera->getTool(fluxeem::ToolType::TOOL_ROI);
if (roi_tool) {
    roi_tool->setParam("x", 100);
    roi_tool->setParam("y", 100);
    roi_tool->setParam("width", 640);
    roi_tool->setParam("height", 480);
    roi_tool->setParam("enable", true);
}
```

## 5. 使用 CMake 编译 tool 配置示例

如果你希望把 tool 调用整理成自己的配置程序，可以使用如下 CMake 工程结构：

```cmake
cmake_minimum_required(VERSION 3.10)
project(tool_config_sample)

set(CMAKE_CXX_STANDARD 20)

find_package(fluxeem_driver CONFIG REQUIRED)

add_executable(tool_config_sample main.cpp)
target_link_libraries(tool_config_sample PRIVATE fluxeem::fluxeem_driver)
```

如果你的工具程序使用图形界面，例如参考 `camera_control_ui`，则还需要额外链接 OpenCV：

```cmake
find_package(OpenCV REQUIRED)
target_link_libraries(tool_config_sample PRIVATE ${OpenCV_LIBS})
target_include_directories(tool_config_sample PRIVATE ${OpenCV_INCLUDE_DIRS})
```

配置和构建示例：

```text
cmake -S . -B build -DCMAKE_PREFIX_PATH=<SDK_INSTALL_DIR>
cmake --build build
```

## 6. 常见工具示例

### 5.1 ROI

ROI 用于限制事件输出区域，常用于：

- 降低数据量
- 聚焦目标区域
- 提高上层算法处理效率

### 5.2 Bias

Bias 用于调节传感器响应特性，不同参数组合会影响事件触发灵敏度与噪声表现。

对于二次开发用户，建议：

- 先在官方示例或调试工具中验证参数效果
- 再将确认过的参数配置固化到自己的业务程序中

### 5.3 Trigger / Sync

Trigger 和 Sync 常用于：

- 外部时序协同
- 触发采集
- 多设备同步

这类参数通常与硬件连接方式强相关，建议在明确硬件接线关系后再进行配置。

## 7. 配置程序建议

如果你需要开发自己的相机配置程序，建议采用以下结构：

1. 启动时枚举设备并打开相机
2. 枚举当前设备支持的工具
3. 根据工具动态生成配置项
4. 配置后回读参数并显示结果
5. 在必要时支持保存和恢复配置

仓库中的 `camera_control_ui` 也可以作为图形化工具配置思路的参考。

## 8. 使用注意事项

- 不同设备支持的 tool 可能不同
- 参数名称、类型和有效范围应以当前设备实际返回结果为准
- 某些配置适合在采集停止状态下调整
- 调参前建议保留默认值或记录变更历史，便于恢复

## 下一步

- @ref recording "学习 file reader 与 RAW 文件处理"
- @ref hardware_sync "学习硬件同步接口和启动流程"

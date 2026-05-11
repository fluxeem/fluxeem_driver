@page python_tool_control_validation 工具参数配置示例验证

@brief 通过 Python 接口验证工具参数读写与控制

本教程用于验证 Fluxeem 工具参数接口在 Python 侧可用，包括 Bias（偏置）、ROI（感兴趣区域）、触发器、抗闪烁、短时去重滤波（Trail Filter）、速率控制、硬件同步等多种工具的参数管理。

## 目标

- 验证各类工具参数可获取（INT/FLOAT/BOOL/STRING/ENUM）
- 验证参数写入与回读正确性
- 验证参数变化对事件输出的影响
- 理解工具参数的交互设置方法

## 1. 运行示例

```bash
cd C:/code/fluxeem_driver_python
python examples/tool_control.py
```

## 2. 建议验证流程

### 基础验证
1. 运行 tool_control.py 连接相机
2. 列出所有可用工具及其参数
3. 记录所有工具的当前参数值（基线）

### 参数调整验证
1. 选择单个工具（如 Bias）
2. 逐个调整参数，每次仅修改一个，进行小步增量
3. 观察效果变化：
   - Bias：背景噪声、边缘清晰度、亮暗响应
   - ROI：兴趣区域是否正确裁剪
   - Rate Control：事件率是否如预期降低
   - Anti-Flicker：闪烁是否得到抑制
4. 回读参数确认硬件实际应用的值

## 3. 工具类型参考

| 工具 | 用途 | 参数示例 |
|------|------|---------|
| TOOL_BIAS | 偏置电压调整 | Bias_n, Bias_p, ... |
| TOOL_ROI | 感兴趣区域 | x0, y0, width, height |
| TOOL_TRIGGER_IN | 外部触发 | trigger_mode, delay, ... |
| TOOL_ANTI_FLICKER | 抗闪烁滤波 | frequency, strength |
| TOOL_TRAIL_FILTER | 同极性短时抑制 | threshold, window_size |
| TOOL_RATE_CONTROL | 事件速率限制 | threshold, period |
| TOOL_SYNC | 硬件同步模式 | sync_mode (MASTER/SLAVE/...) |

## 4. 关键观察项

- Bias 调整：
  - 亮边缘/暗边缘是否更清晰
  - 背景噪声是否明显上升或消除
  - 细节是否被过度抑制

- ROI 配置：
  - 输出事件是否仅在指定区域内
  - 裁剪边界是否正确

- 滤波效果：
  - Rate Control 后事件率是否下降
  - Trail Filter（短时去重滤波）是否有效抑制同像素同极性短时重复事件

## 5. 异常排查

- 参数 set 失败：检查参数名称是否存在、值是否在允许范围内
- 参数 get 返回异常值：确认工具已激活，尝试重新连接相机
- 画面抖动或异常：回退到上一个稳定参数组合，缩小调整步幅
- ENUM 参数设置失败：查看可用选项列表，确保使用正确的枚举值

## 下一步

- @ref python_hardware_sync_validation


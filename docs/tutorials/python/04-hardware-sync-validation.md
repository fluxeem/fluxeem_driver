@page python_hardware_sync_validation 硬件同步示例验证

@brief 验证 Python 接口下的同步模式配置与多设备时间对齐

本教程用于验证同步参数配置是否生效，以及多设备时序是否满足预期。

## 目标

- 验证同步模式设置（STANDALONE/MASTER/SLAVE）
- 验证触发/同步链路连通性
- 验证多设备时间戳对齐效果

## 1. 运行示例

```bash
cd C:/code/fluxeem_driver_python
python examples/hardware_sync.py
```

## 2. 验证点

- 模式切换后可稳定运行
- MASTER 设备输出同步基准，SLAVE 设备可正常跟随
- 多设备事件时间戳偏差在可接受范围内

## 3. 典型问题

- 无法同步：优先检查线缆连接方向和时钟拓扑
- 偶发失步：检查供电稳定性与连接器接触状态

## 下一步

- @ref python_file_playback_validation

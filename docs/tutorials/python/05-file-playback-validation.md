@page python_file_playback_validation RAW 回放示例验证

@brief 使用 Python 验证 RAW 文件读取、回放与基础处理流程

本教程用于验证离线数据处理链路，包括文件打开、时间推进和数据回放。

## 目标

- 验证 RAW 文件可被正确打开
- 验证回放过程稳定可控
- 验证离线处理逻辑可复现

## 1. 运行示例

```bash
cd C:/code/fluxeem_driver_python
python examples/file_playback.py
```

## 2. 验证点

- 文件头信息解析正常
- 回放速度与预期一致（实时/倍速/步进）
- 回放结束后资源可正确释放

## 3. 异常排查

- 打开失败：检查文件路径、权限和文件完整性
- 回放卡顿：检查磁盘吞吐、解码负载与可视化阻塞

## 下一步

- @ref python_slow_motion_validation

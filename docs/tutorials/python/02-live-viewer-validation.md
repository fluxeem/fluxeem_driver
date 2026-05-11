@page python_live_viewer_validation Live viewer示例验证

@brief 通过 Python Live viewer验证事件流接收与基础显示链路

本教程用于验证 Python 侧是否可以稳定接收事件并完成实时显示。

## 目标

- 验证相机打开流程
- 验证事件回调/拉流流程
- 验证基础可视化输出

## 1. 运行示例

使用你的 Python 示例程序（例如 live viewer）启动实时预览。

```bash
cd C:/code/fluxeem_driver_python
python examples/live_viewer.py
```

## 2. 验证点

- 程序能够枚举并打开目标设备
- 程序持续输出事件帧，不出现长时间卡顿
- 画面随场景变化产生稳定响应

## 3. 异常排查

- 黑屏或无数据：检查设备是否被其他进程占用
- 帧率很低：优先检查可视化线程阻塞、分辨率配置与数据处理负载
- 程序退出异常：检查回调线程退出顺序与资源释放

## 下一步

- @ref python_tool_control_validation

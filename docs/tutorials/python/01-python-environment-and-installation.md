@page python_setup Python 环境与安装验证

# Python 环境与安装验证

@brief Python SDK 环境准备、安装与基础连通性验证

本教程用于验证 Python 运行环境是否满足要求，并完成 SDK 安装与最小可用性检查。

## 目标

- 确认 Python 版本与依赖环境可用
- 完成 Python SDK 安装
- 通过最小脚本验证导入与设备访问能力

## 1. 环境要求

- 操作系统：Windows 或 Linux
- Python 版本：建议 3.8 及以上
- 相机驱动：设备已正确连接并可被系统识别

## 2. 创建独立环境（推荐）

```bash
python -m venv .venv
```

激活虚拟环境：

```bash
# Windows
.venv\Scripts\activate

# Linux
source .venv/bin/activate
```

## 3. 安装 Python SDK

根据你的交付方式安装 Python 包（wheel、本地目录或私有源）。示例：

```bash
pip install fluxeem_driver
```

## 4. 安装常用依赖

```bash
pip install numpy opencv-python
```

## 5. 最小验证

```python
import fluxeem

print("fluxeem import ok")
```

若可正常导入，说明 Python SDK 已可用。

## 6. 示例脚本位置

Python 示例脚本位于：

- C:/code/fluxeem_driver_python/examples/

常用脚本：

- live_viewer.py
- tool_control.py
- hardware_sync.py
- file_playback.py
- slow_motion.py

## 7. 常见问题

- 导入失败：确认当前解释器与安装位置一致
- 找不到设备：确认 USB 连接、权限和驱动状态
- 运行崩溃：优先检查 Python 包版本与 SDK 版本是否匹配

## 下一步

- @ref python_live_viewer_validation


<div align="center">
  <img src="docs/img/html_title.png" alt="Fluxeem" width="260" />
</div>

<h1 align="center">Fluxeem Driver</h1>

<p align="center">
  <strong>事件相机驱动与 SDK</strong> — 设备发现 · 事件采集 · 参数配置 · 文件回放 · 跨平台打包
</p>
<p align="center">
  <a href="https://fluxeem.github.io/fluxeem_driver/">在线文档</a> &middot;
  <a href="https://www.fluxeem.com">官网</a>
</p>
<p align="center">
  <a href="https://github.com/fluxeem/fluxeem_driver/releases"><img src="https://img.shields.io/github/v/release/fluxeem/fluxeem_driver" alt="Release"></a>
  <a href="https://github.com/fluxeem/fluxeem_driver/blob/main/LICENSE"><img src="https://img.shields.io/github/license/fluxeem/fluxeem_driver" alt="License"></a>
  <a href="https://fluxeem.github.io/fluxeem_driver/"><img src="https://img.shields.io/badge/docs-GitHub%20Pages-brightgreen" alt="Docs"></a>
</p>


## 核心能力

| 模块 | 说明 |
| :--- | :--- |
| 设备管理 | USB 事件相机自动发现、打开、关闭和生命周期管理 |
| 事件采集 | 实时事件流采集、回调通知与同步批次获取 |
| 参数工具 | 偏置 / ROI / 触发 / 同步 / 抗闪烁 / 事件轨迹滤波 / 事件率控制 |
| 文件回放 | RAW 文件读取、离线回放和时序检索 |
| 示例工具 | Live Viewer · Camera Control UI · Hardware Sync Demo |

## 快速开始

### 方式 A — 安装预编译包

适用于：直接使用 SDK、运行示例、验证相机或基于已安装 SDK 进行二次开发。

1. 从 [GitHub Releases](https://github.com/fluxeem/fluxeem_driver/releases) 下载安装包，或在 Debian / Ubuntu 系统上通过 APT 仓库安装
2. 阅读 [在线文档](https://fluxeem.github.io/fluxeem_driver/) 获取集成指引
3. 参考教程：[C++ 快速入门](https://fluxeem.github.io/fluxeem_driver/tutorial_cpp_index.html) | [Python 快速入门](https://fluxeem.github.io/fluxeem_driver/tutorial_python_index.html)

### 方式 B — 从源码构建

适用于：修改驱动源码、维护示例、生成安装包、调试跨平台构建或参与发布。

> **前置条件**：CMake ≥ 3.10 · C++20 编译器 · libusb-1.0

## 仓库结构

```
include/      公开 API 头文件
src/          驱动库实现
examples/     C++ 示例程序
app/          附加工具（camera_control_ui）
tests/        GoogleTest 单元测试
docs/         教程与 Doxygen 文档
cmake/        CMake 模块与 Doxygen 模板
installer/    Windows / Ubuntu 安装脚本与 USB 驱动
third_party/  随源码维护的第三方组件
```

## 平台与产物

| 平台 | 产物 | 工具链 |
| :--- | :--- | :--- |
| Windows x64 | NSIS `.exe` 安装包 | MSVC + vcpkg |
| Debian/Ubuntu (amd64) | `.deb` 安装包 | GCC（基于 Ubuntu 20.04，glibc ≥ 2.31） |

CI 在 GitHub Actions 中构建上述产物。

## 构建选项

| 选项 | 默认 | 说明 |
| :--- | :--- | :--- |
| `BUILD_SAMPLES` | `OFF` | 构建 examples/ 和 app/camera_control_ui/ |
| `BUILD_TESTING` | `OFF` | 构建 tests/ |
| `GENERATE_DOCS` | `OFF` | 启用 Doxygen 文档目标 |
| `DOXYGEN_AWESOME_DIR` | 空 | doxygen-awesome-css 仓库根目录 |
| `UDEV_RULES_SYSTEM_INSTALL` | `ON` | (Linux) cmake --install 时将 udev 规则写入 /etc/udev/rules.d/ |

## Windows 构建步骤

推荐使用 vcpkg + Visual Studio 2022。

<details>
<summary>1. 准备 vcpkg</summary>

```powershell
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
[Environment]::SetEnvironmentVariable("VCPKG_ROOT", "C:\path\to\vcpkg", "User")
```

设置完成后重新打开终端或 IDE。

</details>

<details>
<summary>2. 安装依赖</summary>

仅构建驱动库：

```powershell
vcpkg install libusb:x64-windows
```

构建驱动库 + 示例 + 测试：

```powershell
vcpkg install libusb:x64-windows opencv:x64-windows gtest:x64-windows
```

</details>

<details>
<summary>3. 配置 & 构建 & 安装</summary>

```powershell
cmake -S . -B build `
  -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake `
  -DBUILD_SAMPLES=ON

cmake --build build --config Release --parallel
cmake --install build --config Release --prefix install
```

</details>

<details>
<summary>4. 安装 USB 驱动（首次使用）</summary>

以**管理员**身份运行（需要 `wdi-simple.exe`，见 [libwdi](https://github.com/pbatard/libwdi)）：

```powershell
wdi-simple.exe -n "ApexVision-S1"    -m "Fluxeem" -v 0x04b4 -p 0x0101
wdi-simple.exe -n "EVK4"             -m "Fluxeem" -v 0x04b4 -p 0x00f5
wdi-simple.exe -n "RDK3"             -m "Fluxeem" -v 0x04b4 -p 0x00f4
wdi-simple.exe -n "SENSING_USB3_EVS_CAMERA" -m "Fluxeem" -v 0x04b4 -p 0x00c4
wdi-simple.exe -n "DVSLume"          -m "Fluxeem" -v 0x04b5 -p 0x0001
```

</details>

<details>
<summary>5. 生成 NSIS 安装包</summary>

```powershell
winget install NSIS.NSIS
cmake --build build --config Release --parallel
cd build
cpack -C Release -G NSIS
```

</details>

## Linux 构建步骤

以 Ubuntu 为例。

<details>
<summary>安装依赖 & 构建</summary>

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake pkg-config libusb-1.0-0-dev libopencv-dev libgtest-dev file

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_SAMPLES=ON
cmake --build build --parallel
cmake --install build --prefix install
```

</details>

<details>
<summary>运行测试</summary>

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_SAMPLES=ON -DBUILD_TESTING=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

</details>

<details>
<summary>生成 DEB 安装包</summary>

```bash
cd build && cpack -G DEB
```

</details>

## 安装集成

安装后目录布局：

| 路径 | 内容 |
| :--- | :--- |
| `include/` | 公开头文件 |
| `bin/` | 动态库、可执行程序 |
| `lib/` | 库文件 |
| `share/cmake/fluxeem_driver/` | CMake 包配置 |
| `share/licenses/fluxeem_driver/` | 许可证 |
| `examples/` | 示例源码 |

下游 CMake 工程集成方式：

```cmake
find_package(fluxeem_driver CONFIG REQUIRED)
target_link_libraries(your_app PRIVATE fluxeem::fluxeem_driver)
```

## 示例程序一览

| 目标 | 说明 | 依赖 |
| :--- | :--- | :--- |
| `logger_sample` | 基础日志接口 | 无 |
| `loguru_sample` | Loguru 日志集成 | 无 |
| `fluxeem_live_viewer` | 实时事件流可视化 | OpenCV |
| `raw_file_reader_sample` | RAW 文件读取与回放 | OpenCV |
| `slow_motion_playback_sample` | 慢动作回放 | OpenCV |
| `camera_hardware_sync_sample` | 硬件同步 | OpenCV |
| `camera_control_ui` | 相机工具参数配置界面 | OpenCV |

## 常见问题

<details>
<summary>Windows 找不到 libusb-1.0.lib</summary>

确认：
- 已安装 `libusb:x64-windows`
- 已设置 `VCPKG_ROOT`
- CMake 配置命令包含 `-DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake`
- 修改依赖后已删除旧的 `build/` 目录并重新配置

</details>

<details>
<summary>示例程序找不到 OpenCV</summary>

启用 `BUILD_SAMPLES=ON` 时，Live Viewer、RAW 回放、硬件同步示例和 `camera_control_ui` 需要 OpenCV。请安装 OpenCV 开发包，或关闭 `BUILD_SAMPLES` 仅构建驱动库。

</details>

<details>
<summary>Linux CPack DEB 依赖扫描失败</summary>

需安装 `file` 工具：`sudo apt-get install -y file`

</details>

<details>
<summary>文档生成失败</summary>

确认已安装 Doxygen 和 Graphviz，`DOXYGEN_AWESOME_DIR` 指向 `doxygen-awesome-css` 仓库根目录且其下存在 `doxygen-awesome.css`。

</details>

## 许可证

Apache-2.0，详见 [LICENSE](LICENSE)。

<div align="center">
  <img src="docs/img/html_title.png" alt="Fluxeem Logo" width="260" />
</div>


<h1 align="center">Fluxeem Driver</h1>

<p align="center">
  面向事件相机的驱动与 SDK，提供设备连接、事件流采集、工具配置、文件回放与跨平台打包能力。
</p>
<p align="center">
  <a href="https://fluxeem.github.io/fluxeem_driver/">在线文档</a> ·
  <a href="https://www.fluxeem.com">官网</a>
</p>
<p align="center">
  <a href="https://github.com/fluxeem/fluxeem_driver/releases"><img src="https://img.shields.io/github/v/release/fluxeem/fluxeem_driver" alt="Release"></a>
  <a href="https://github.com/fluxeem/fluxeem_driver/blob/main/LICENSE"><img src="https://img.shields.io/github/license/fluxeem/fluxeem_driver" alt="License"></a>
  <a href="https://fluxeem.github.io/fluxeem_driver/"><img src="https://img.shields.io/badge/docs-GitHub%20Pages-brightgreen" alt="Docs"></a>
</p>


## 主要功能

- 事件相机设备发现、打开、关闭和生命周期管理
- 实时事件流采集、回调和基础数据处理
- 相机参数、工具接口和硬件同步相关能力
- RAW 文件读取、回放和离线处理接口
- C++ 示例程序、Live Viewer 和工具配置界面

## 使用简介
### 1. 使用安装包

如果你只需要安装 Fluxeem Driver SDK、运行示例程序、验证相机功能或基于已安装的 SDK 进行二次开发，请优先阅读 [docs/README.md](docs/README.md)。

推荐入口：

- [fluxeem_driver](https://fluxeem.github.io/fluxeem_driver/)：完整文档导航
- [C++ 使用教程](https://fluxeem.github.io/fluxeem_driver/tutorial_cpp_index.html)：C++ SDK 快速开始
- [python 使用教程](https://fluxeem.github.io/fluxeem_driver/tutorial_python_index.html)：python 快速开始

安装包用户通常不需要关心源码目录结构、CMake 配置细节或第三方依赖编译方式。安装完成后，请根据教程完成环境验证、设备连接、示例运行和 SDK 接入。

### 2. 从源码构建使用

如果你需要修改驱动源码、维护示例程序、生成安装包、调试跨平台构建或参与 SDK 发布，请继续阅读本文档中的构建说明。

- [构建要求](#构建要求)
- [Windows 源码编译](#Windows源码编译)
- [Linux 源码编译](#Linux源码编译)

## 仓库结构

| 目录 | 说明 |
| --- | --- |
| [include/](include/) | 对外公开头文件和 SDK API |
| [src/](src/) | 驱动库实现代码 |
| [examples/](examples/) | C++ 示例程序 |
| [app/](app/) | 附加应用程序，目前包含 `camera_control_ui` |
| [tests/](tests/) | GoogleTest 测试代码 |
| [docs/](docs/) | 用户文档、教程、Doxygen 首页和静态资源 |
| [cmake/](cmake/) | CMake 模块、包配置模板和 Doxygen 模板 |
| [installer/](installer/) | Windows / Ubuntu 安装辅助脚本和驱动资源 |
| [third_party/](third_party/) | 随源码维护的第三方组件 |

## 支持平台与产物

当前工程主要面向以下平台构建：

| 平台 | 典型产物 | 说明 |
| --- | --- | --- |
| Windows x64 | `.exe` 安装包 | 使用 CMake、MSVC、vcpkg 和 NSIS 构建 |
| Ubuntu 20.04 / 22.04 / 24.04 | `.deb` 安装包 | 使用 CMake、系统依赖和 CPack DEB 构建 |

CI 会在 GitHub Actions 中构建 Windows 安装包和 Ubuntu DEB 包。

## 构建要求

基础要求：

- CMake 3.10 或更高版本
- 支持 C++20 的编译器
- libusb-1.0

可选依赖：

- OpenCV：构建 Live Viewer、RAW 回放示例和 `camera_control_ui`
- GoogleTest：构建测试
- Doxygen：生成 API 文档
- Graphviz：生成 Doxygen 图形化关系图
- doxygen-awesome-css：生成当前样式的文档站点

## CMake 选项

| 选项 | 默认值 | 说明 |
| --- | --- | --- |
| `BUILD_SAMPLES` | `OFF` | 构建 [examples/](examples/) 和 [app/camera_control_ui/](app/camera_control_ui/) |
| `BUILD_TESTING` | `OFF` | 构建 [tests/](tests/) |
| `GENERATE_DOCS` | `OFF` | 启用 Doxygen 文档目标 |
| `DOXYGEN_AWESOME_DIR` | 空 | `doxygen-awesome-css` 仓库根目录 |
| `UDEV_RULES_SYSTEM_INSTALL` | `ON` | （仅 Linux）`cmake --install` 时将 udev 规则安装到 `/etc/udev/rules.d/`，需以 root 运行 |

## Windows源码编译

Windows 下推荐使用 vcpkg 管理依赖，使用 Visual Studio 2022 生成器构建。

### 1. 准备 vcpkg

```powershell
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
```

设置 `VCPKG_ROOT` 环境变量：

```powershell
[Environment]::SetEnvironmentVariable("VCPKG_ROOT", "C:\path\to\vcpkg", "User")
```

设置完成后请重新打开终端或 IDE。

### 2. 安装依赖

仅构建驱动库：

```powershell
vcpkg install libusb:x64-windows
```

构建驱动库、示例和测试：

```powershell
vcpkg install libusb:x64-windows opencv:x64-windows gtest:x64-windows
```

### 3. 配置工程

构建驱动库和示例程序：

```powershell
cmake -S . -B build `
  -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake `
  -DBUILD_SAMPLES=ON
```

如需同时构建测试：

```powershell
cmake -S . -B build `
  -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake `
  -DBUILD_SAMPLES=ON `
  -DBUILD_TESTING=ON
```

### 4. 编译与安装

```powershell
cmake --build build --config Release --parallel
cmake --install build --config Release --prefix install
```

### 5. 安装 USB 驱动

首次连接设备前，需要为每款设备安装 WinUSB 驱动。请以**管理员**身份运行以下命令（需要 `wdi-simple.exe`，可从 [libwdi](https://github.com/pbatard/libwdi) 获取）：

```powershell
# ApexVision-S1  (VID_04B4 / PID_0101)
wdi-simple.exe -n "ApexVision-S1" -m "Fluxeem" -v 0x04b4 -p 0x0101
# EVK4  (VID_04B4 / PID_00F5)
wdi-simple.exe -n "EVK4" -m "Fluxeem" -v 0x04b4 -p 0x00f5

# RDK3  (VID_04B4 / PID_00F4)
wdi-simple.exe -n "RDK3" -m "Fluxeem" -v 0x04b4 -p 0x00f4

# SENSING_USB3_EVS_CAMERA  (VID_04B4 / PID_00C4)
wdi-simple.exe -n "SENSING_USB3_EVS_CAMERA" -m "Fluxeem" -v 0x04b4 -p 0x00c4

# DVSLume  (VID_04B5 / PID_0001)
wdi-simple.exe -n "DVSLume" -m "Fluxeem" -v 0x04b5 -p 0x0001
```

只需为实际使用的设备执行对应命令即可。

### 6. 生成 Windows 安装包

生成 NSIS 安装包前需要先安装 NSIS。

```powershell
winget install NSIS.NSIS
cmake --build build --config Release --parallel
cd build
cpack -C Release -G NSIS
```

生成的 `.exe` 安装包位于 `build/` 目录。

## Linux源码编译

以下命令以 Ubuntu 为例。

### 1. 安装依赖

```bash
sudo apt-get update
sudo apt-get install -y \
  build-essential \
  cmake \
  pkg-config \
  libusb-1.0-0-dev \
  libopencv-dev \
  libgtest-dev \
  file
```

### 2. 配置、编译与安装

```bash
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_SAMPLES=ON

cmake --build build --parallel
cmake --install build --prefix install
```

如需构建测试：

```bash
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_SAMPLES=ON \
  -DBUILD_TESTING=ON

cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

### 3. 生成 DEB 安装包

```bash
cd build
cpack -G DEB
```

生成的 `.deb` 安装包位于 `build/` 目录。

## 安装结果

执行 `cmake --install` 或安装发布包后，典型目录内容如下：

| 目录 | 内容 |
| --- | --- |
| `include/` | 公开头文件 |
| `bin/` | 动态库、可执行程序和运行时文件 |
| `lib/` | 库文件 |
| `share/cmake/fluxeem_driver/` | CMake 包配置文件 |
| `share/licenses/fluxeem_driver/` | 许可证文件 |
| `examples/` | 随 SDK 提供的示例源码 |

下游 CMake 工程可以通过以下方式集成：

```cmake
find_package(fluxeem_driver CONFIG REQUIRED)
target_link_libraries(your_app PRIVATE fluxeem::fluxeem_driver)
```

## 示例程序

启用 `BUILD_SAMPLES=ON` 后会构建以下示例和工具：

| 目标 | 说明 | 额外依赖 |
| --- | --- | --- |
| `logger_sample` | 基础日志接口示例 | 无 |
| `loguru_sample` | Loguru 集成示例 | 无 |
| `fluxeem_live_viewer` | 实时事件流显示示例 | OpenCV |
| `raw_file_reader_sample` | RAW 文件读取与回放示例 | OpenCV |
| `slow_motion_playback_sample` | 慢动作回放示例 | OpenCV |
| `camera_hardware_sync_sample` | 硬件同步示例 | OpenCV |
| `camera_control_ui` | 相机工具和参数配置界面 | OpenCV |

更完整的示例使用说明请阅读 [docs/tutorials/cpp/](docs/tutorials/cpp/) 下的教程。

## 测试

启用 `BUILD_TESTING=ON` 后可运行测试：

```bash
ctest --test-dir build --output-on-failure
```

Windows 多配置生成器建议指定配置：

```powershell
ctest --test-dir build -C Release --output-on-failure
```

测试覆盖基础类型、工具接口、对象池、文件读取和部分相机相关逻辑。涉及真实设备的测试可能需要连接相机并准备对应硬件环境。



## 常见问题

### Windows 找不到 `libusb-1.0.lib`

通常是 vcpkg 没有正确接入。请确认：

- 已安装 `libusb:x64-windows`
- 已设置 `VCPKG_ROOT`
- CMake 配置命令包含 `-DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake`
- 修改依赖后已删除旧的 `build/` 目录并重新配置

### 示例程序找不到 OpenCV

启用 `BUILD_SAMPLES=ON` 时，Live Viewer、RAW 回放、硬件同步示例和 `camera_control_ui` 需要 OpenCV。请安装对应平台的 OpenCV 开发包，或关闭 `BUILD_SAMPLES` 仅构建驱动库。

### Linux 打包时 CPack DEB 依赖扫描失败

如果启用了 DEB 的共享库依赖扫描，需要系统中存在 `file` 工具。请安装：

```bash
sudo apt-get install -y file
```

### 文档生成失败

请确认：

- 已安装 Doxygen
- 已安装 Graphviz
- `DOXYGEN_AWESOME_DIR` 指向 `doxygen-awesome-css` 仓库根目录
- `DOXYGEN_AWESOME_DIR` 下存在 `doxygen-awesome.css`

## 许可证

本项目使用 Apache-2.0 许可证，详见 [LICENSE](LICENSE)。


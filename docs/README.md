# Fluxeem Driver 文档导航

本页汇总 Fluxeem Driver SDK 的教程文档、API 参考和推荐阅读路径，供新成员和 SDK 使用方快速定位。

## 推荐阅读顺序

1. [仓库首页说明](../README.md)：了解依赖、构建方式、示例和安装导出方式
2. [C++ 教程目录](tutorials/cpp/_sidebar.md)：按 C++ 开发流程学习 SDK 使用
3. [Python 教程目录](tutorials/python/README.md)：按 Python 集成流程学习 SDK 使用
4. [01-sdk-quickstart](tutorials/cpp/01-sdk-quickstart.md)：完成 SDK 安装、环境验证和首次接入
5. [02-device-discovery-and-validation](tutorials/cpp/02-device-discovery-and-validation.md)：学习设备发现与环境验证
6. [03-live-viewer-and-basic-apis](tutorials/cpp/03-live-viewer-and-basic-apis.md)：学习Live viewer和基础接口
7. [04-tool-interface-and-configuration](tutorials/cpp/04-tool-interface-and-configuration.md)：了解 tool 接口和参数配置
8. [05-file-reader-and-raw-processing](tutorials/cpp/05-file-reader-and-raw-processing.md)：学习 RAW 文件读取和处理
9. [06-tool-demo-and-configuration](tutorials/cpp/06-tool-demo-and-configuration.md)：学习 tool 示例程序的组织方式
10. [07-hardware-sync-and-hal-sync](tutorials/cpp/07-hardware-sync-and-hal-sync.md)：学习硬件同步与 HAL Sync

## 文档组成

### 1. 教程文档

教程源码位于 docs/tutorials/，同时被 Doxygen 纳入最终站点：

- cpp/_sidebar.md：C++ 教程入口页
- cpp/01-sdk-quickstart.md：SDK 安装、环境验证和首次接入
- cpp/02-device-discovery-and-validation.md：设备发现与环境验证
- cpp/03-live-viewer-and-basic-apis.md：Live viewer与基础接口
- cpp/04-tool-interface-and-configuration.md：Tool 接口与参数配置
- cpp/05-file-reader-and-raw-processing.md：File Reader 与 RAW 数据处理
- cpp/06-tool-demo-and-configuration.md：Tool 示例程序说明
- cpp/07-hardware-sync-and-hal-sync.md：硬件同步与 HAL Sync
- python/README.md：Python 教程入口页

### 2. API 文档

公开 API 主要来自以下头文件：

- include/driver/camera/ev_camera_service.hpp
- include/driver/camera/base/i_camera.hpp
- include/driver/file_reader/ev_file_reader.h
- include/hal/tools/camera_tool.h
- include/hal/tools/tool_info.h

建议优先关注：

- fluxeem::EvCameraService：发现、列举和打开设备
- fluxeem::ICamera：相机生命周期、事件回调和工具访问
- fluxeem::EvFileReader：RAW 文件读取与回放

## 如何生成站点文档

在仓库根目录执行：
```powershell
cmake -S . -B build `
  -DGENERATE_DOCS=ON `
  -DDOXYGEN_AWESOME_DIR=C:\path\to\doxygen-awesome-css
cmake --build build --target docs
```

生成后的首页位于：

```text
build/docs/html/index.html
```

## 面向不同读者的入口

### SDK 使用者

先看：

- [../README.md](../README.md)
- [tutorials/cpp/01-sdk-quickstart.md](tutorials/cpp/01-sdk-quickstart.md)
- [tutorials/cpp/03-live-viewer-and-basic-apis.md](tutorials/cpp/03-live-viewer-and-basic-apis.md)
- [tutorials/python/README.md](tutorials/python/README.md)

### 示例维护者

重点看：

- `examples/fluxeem_live_viewer/`
- `examples/raw_file_reader_sample/`
- `app/camera_control_ui/`

这些目标都依赖 OpenCV，构建前需要先满足 `find_package(OpenCV REQUIRED)`。

### 文档维护者

补充或修改教程时，请同步关注：

- Markdown 文件是否能单独阅读
- 示例代码是否与当前公开 API 一致
- README 中的构建命令是否仍然可执行
- 新增依赖是否已写入仓库首页

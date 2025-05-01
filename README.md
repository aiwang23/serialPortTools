# ![icon.png](doc/icon.png) serialPortTools

![GitHub License](https://img.shields.io/github/license/mashape/apistatus)

## 简介

本项目是一个串口助手

## 程序截图

![img.png](doc/light.png)
![img.png](doc/dark.png)
> 目前完美支持浅色模式和深色模式

## 使用文档

### 串口客户端

#### 整体布局

![overall_interface.png](doc/serialClient_overall_interface.png)

#### 接收区设置

![receiver_area_settings.png](doc/serialClient_receiver_area_settings.png)

> 注意: Markdown模式和 HTML模式一定要接收到正确的格式才会正常显示

#### 发送区设置

![send_zone_Sending_mode.png](doc/serialClient_send_zone_Sending_type.png)

![send_zone_Sending_mode.png](doc/serialClient_send_zone_Sending_mode.png)

### 串口服务器

#### 整体布局

![Snipaste_2025-04-19_16-24-23.png](doc/serialServer_overall_interface.png)

## 支持平台

| Windows      | Debian          |
|--------------|-----------------|
| ![win-badge] | ![Debian-badge] |

> 在Windows11 和 Debian12 和 KUbuntu2404 上成功编译 <br>
> Ubuntu 应该也可以, 还没试

[win-badge]: https://img.shields.io/badge/Windows-Passing-61C263

[Debian-badge]: https://img.shields.io/debian/v/apt

## 使用的第三方库和对应的版本

- [Qt](https://github.com/qt) v6.4
- [ElaWidgetTools](https://github.com/Liniyous/ElaWidgetTools) Latest
- [itas109::CSerialPort](https://github.com/itas109/CSerialPort) Latest
- [maddy](https://github.com/progsource/maddy) v1.3.0
- [concurrentqueue](https://github.com/cameron314/concurrentqueue) v1.0.4
- [ThreadPool](https://github.com/progschj/ThreadPool) Latest
- [nlohmann::json](https://github.com/nlohmann/json) v3.11.3
- [libssh2](https://github.com/libssh2/libssh2) Latest

## 拉取该项目,并加载子模块

```shell
git clone --recursive https://github.com/aiwang23/serialPortTools.git
```

## 编译

> 1. 请把[CMakeLists.txt](CMakeLists.txt)的 `CMAKE_PREFIX_PATH` 修改自己电脑的Qt安装路径
>  2. 请自己电脑上安装 **openssl**, 并设置环境变量
>     1. Windows, 可使用 [openssl](https://slproweb.com/products/Win32OpenSSL.html) 来快速安装
>     2. Debian/Ubuntu 可使用 `sudo apt install libssl-dev` 来快速安装

### Windows10+

> windows 编译器使用 msvc2022
> debian 编译器使用 gcc/g++

```shell
cmake -S . -B ./build/
cmake --build ./build/
```

## 国际化

目前支持简体中文和英语的切换, 后续后添加更多语言支持, 有其他语言需求的, 请自行配置语言

### 生成 ts文件

```powershell
lupdate -recursive . -ts res/translations/zh_CN.ts res/translations/en_US.ts
```

### ts文件 -> qm文件

```
lrelease res/translations/zh_CN.ts res/translations/en_US.ts
```

然后在 [resources.qrc](resources.qrc) 添加 qm文件路径


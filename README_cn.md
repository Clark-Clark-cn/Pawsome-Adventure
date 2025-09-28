# Pawsome Adventure (Hajimi Adventure)

一个多人（含团队模式）文字竞速游戏，包含跨平台 HTTP 服务器（C++17 + cpp-httplib）和 Windows 客户端（基于 EasyX，CMake 中暂未包含）。服务器负责文字分发、心跳同步、进度汇总；客户端负责渲染和交互。

## **开始游戏**
- 运行服务端
- 运行客户端（至少2个客户端或2组客户端）

## 特性

- 跨平台服务器：Windows / Linux / macOS（CMake 构建）
- 团队/单人模式，可配置团队大小
- 文字按团队成员分段分发（同一文字的不同段落）
- 心跳同步，自动清理超时玩家
- 简单 REST API，便于集成或自测
- GitHub Actions CI（三平台构建 + 产物上传）

## 仓库结构

```
Pawsome-Adventure/
├─ CMakeLists.txt                # 顶层 CMake（转发到子目录）
├─ README-cmake.md               # 英文构建说明（仅服务器）
├─ README.md / README_cn.md      # 项目描述（你正在读这个）
├─ vcpkg.json                    # 依赖声明（cpp-httplib）
├─ .github/workflows/ci.yml      # CI：Windows/Linux/macOS 构建并上传产物
├─ Pawsome-Adventure/            # 源码（服务器/客户端，共享头文件）
│  ├─ CMakeLists.txt             # 子项目 CMake（默认构建服务器）
│  ├─ server.cpp                 # 服务器入口（跨平台）
│  ├─ client.cpp                 # 客户端（Windows + EasyX）
│  ├─ config.h                   # 简单配置系统（服务器/客户端共享）
│  ├─ ... 其他头文件（动画/渲染/路径/计时等）
├─ server/                       # 服务器默认资源
│  ├─ server.ini                 # 服务器配置（首次运行时保存/更新）
│  └─ text.txt                   # 演示文字
└─ resources/                    # 客户端资源（图片/字体/音频）（Windows）
```

## 前置条件

- CMake 3.16+
- C++17 编译器
  - Windows：Visual Studio 2022 (MSVC) 或 MinGW-w64
  - Linux：GCC / Clang
  - macOS：Apple Clang (Xcode Command Line Tools)
- 依赖：cpp-httplib（仅头文件）
  - 默认：通过 FetchContent 自动获取
  - 或通过 vcpkg/apt/brew 安装并提供包含路径

## 快速开始（构建服务器）

默认仅构建服务器目标 `pawsome_server`。

- Windows (MSVC, x64)

```cmd
cmake -S . -B build -A x64 -DUSE_FETCHCONTENT_HTTPLIB=ON
cmake --build build --config Release -- /m
```

- Linux/macOS

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DUSE_FETCHCONTENT_HTTPLIB=ON
cmake --build build -j
```

构建后，`pawsome_server` 可执行文件目录会自动复制 `server/server.ini` 和 `server/text.txt`，便于运行。

可选：使用 vcpkg（代替 FetchContent）

- Windows（示例）

```cmd
cmake -S . -B build -A x64 -DUSE_FETCHCONTENT_HTTPLIB=OFF -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake
cmake --build build --config Release -- /m
```

- Linux/macOS（示例）

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DUSE_FETCHCONTENT_HTTPLIB=OFF -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake
cmake --build build -j
```

## 运行服务器

在构建输出目录（包含 `server.ini`/`text.txt`）中运行：

- Windows

```cmd
.\pawsome_server.exe
```

- Linux/macOS

```sh
./pawsome_server
```

默认监听 `0.0.0.0:8888`。首次运行会将默认配置写回 `server.ini`。

## 配置（server.ini）

示例：

```
port=8888
team_mode=false
text_file=text.txt
heartbeat_timeout=10
team_size=2
server=http://127.0.0.1:8888
```

- port：监听端口
- team_mode：启用团队模式（true/false）
- team_size：团队大小（>=1）
- heartbeat_timeout：心跳超时秒数
- text_file：分发的文字文件（相对于工作目录）
- server：客户端默认服务器地址（客户端使用）

注意：`text_file` 路径相对于运行时工作目录。若从其他目录启动，使用绝对路径或确保文件存在。

## REST API

服务器通过 cpp-httplib 提供简单 API（Content-Type: text/plain）：

- POST `/login`
  - 响应：`<id>,<teamID>,<memberIndex>,<teamSize>`
  - 描述：返回玩家 ID 和分配的团队信息；服务器根据 `team_mode`/`team_size` 分配

- POST `/queryText`（可选参数：`id`）
  - 无 `id`：返回完整文字
  - 有 `id`：若团队模式，返回该成员的段落；否则完整文字

- POST `/sync`（参数：`id`, `progress`）
  - 上报玩家进度（整数，自定义语义）
  - 响应：类似 `pid:progress:teamID:segmentLen;...` 的列表
  - 同时刷新心跳；服务器清理超过 `heartbeat_timeout` 的玩家

- POST `/exit`（参数：`id`）
  - 退出并移除玩家，返回 `ok`

- GET `/mode`
  - 响应：`team,<size>` 或 `solo,<size>`

- POST `/setMode`（参数：`mode` 为 `team|solo|true|false|1|0`；可选 `teamSize`）
  - 更改模式和团队大小；清空在线玩家，重置 ID
  - 同步回 `server.ini`

快速自测（示例）

```sh
# 登录（返回 id,teamID,memberIndex,teamSize）
curl -X POST http://127.0.0.1:8888/login

# 查询文字（有 id 返回段落）
curl -X POST "http://127.0.0.1:8888/queryText?id=1"

# 上报进度
curl -X POST "http://127.0.0.1:8888/sync?id=1&progress=123"

# 查询/设置模式
curl http://127.0.0.1:8888/mode
curl -X POST "http://127.0.0.1:8888/setMode?mode=team&teamSize=3"
```

安全提示：此服务仅供本地/内网演示，无认证/限流/持久化；请勿暴露到公网。

## 客户端（Windows/EasyX）

- 源码：`Pawsome-Adventure/client.cpp` 及相关头文件，`resources/` 资产
- 依赖：EasyX，WinMM/MSIMG32（见 `util.h` 中的 `#pragma comment`）
- CMake 构建中暂未包含；构建时手动在 VS 中配置，或告诉我添加 `BUILD_CLIENT` 开关（仅 Windows）

## 持续集成（CI）

- 工作流：`.github/workflows/ci.yml`
- 触发：push / pull_request 到 `main`，或手动
- 平台：Windows/Linux/macOS
- 产物上传：
  - Windows：`pawsome_server.exe` + `server.ini` + `text.txt`
  - Linux/macOS：`pawsome_server` + `server.ini` + `text.txt`
- 构建选项：默认 `-DUSE_FETCHCONTENT_HTTPLIB=ON`

从仓库 Actions 页面下载对应平台产物。

## FAQ

- 构建失败，找不到 `httplib.h`
  - 默认 `USE_FETCHCONTENT_HTTPLIB=ON` 自动获取；若关闭，通过 vcpkg/apt/brew 安装并提供包含路径（或使用 vcpkg 工具链）
- 运行时错误 "Failed to open text: ..."
  - 检查 `server.ini` `text_file` 路径，确保相对于工作目录，或使用绝对路径
- 端口被占用
  - 更改 `server.ini` 中的 `port`，或杀死占用该端口的进程
- Linux/macOS 缺少线程库
  - CMake 通过 `find_package(Threads)` 处理；若失败，确保工具链/头文件完整
- Windows 控制台乱码
  - 尝试切换终端编码，或仅查看英文日志（当前日志多为英文/数字）

## 开发与贡献

- 使用 CMake 统一构建；推荐 `-DUSE_FETCHCONTENT_HTTPLIB=ON`
- PR 应包含描述和平台验证
- 若将客户端集成到 CMake，请指定 EasyX 获取方式（安装/包含路径/库）

## License

暂无许可证声明（保留所有权利）。分发或商业使用请联系仓库所有者。
Linux/macOS
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DUSE_FETCHCONTENT_HTTPLIB=ON
cmake --build build -j
```
构建完成后，pawsome_server 可执行文件所在目录会自动复制 server.ini 和 text.txt 方便直接运行。

可选：使用 vcpkg（替代 FetchContent）
```cmd
cmake -S . -B build -A x64 -DUSE_VCPKG=ON
cmake --build build --config Release -- /m
```
Linux/macOS
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DUSE_VCPKG=ON
cmake --build build -j
```
构建完成后，pawsome_server 可执行文件所在目录会自动复制 server.ini 和 text.txt 方便直接运行。

### 运行
Windows
```cmd
.\pawsome_server.exe
```
Linux/macOS
```bash
./pawsome_server
```
默认监听 8888 端口，可通过 server.ini 修改。

## 配置（server.ini）
```ini
port=8888                     ; 服务器监听端口
team_mode=false               ; 是否启用组队模式（true/false）
text_file=text.txt            ; 文本文件路径
heartbeat_timeout=10          ; 心跳超时时间（秒）
team_size=2                   ; 组队人数（仅在组队模式下生效）
server=http://localhost:8888  ; 服务器地址（客户端用）
```
## 客户端（Windows）
### 先决条件
- Visual Studio 2022（MSVC）
- EasyX 图形库（含头文件和链接库）
- 依赖：cpp-httplib(vcpkg 获取)

### 构建
- 打开sln文件
- 选择“生成”菜单，然后选择“生成解决方案”
- 生成完成后，pawsome_client.exe 可执行文件位于 build\Release 目录下
- 将 resources 目录复制到 build\Release 目录下
- 运行 pawsome_adventure.exe

### 运行
```cmd
.\pawsome_client.exe
```
默认连接到 http://localhost:8888
可通过 client.ini 修改。
## 配置（client.ini）
```ini
server=http://localhost:8888  ; 服务器地址（默认）  
```

## API
服务端基于 cpp-httplib 提供简单 API（Content-Type 均为 text/plain）：

1. POST /login

    响应：<id>,<teamID>,<memberIndex>,<teamSize>
    说明：返回玩家 ID 和分配的队伍信息，server 端根据 team_mode/team_size 统一分配
2. POST /queryText（参数可选：id）

    无 id：返回完整文本
    有 id：若处于组队，返回该成员负责的文本片段，否则仍返回完整文本
3. POST /sync（参数：id, progress）

    上报玩家进度（整数，自定义语义）
    响应：形如 pid:progress:teamID:segmentLen;... 的列表
    同时刷新心跳；服务端会清理超过 heartbeat_timeout 未同步的玩家
4. POST /exit（参数：id）

    退出并移除玩家，返回 ok
5. GET /mode

    响应：team,<size> 或 solo,<size>
6. POST /setMode（参数：mode，可为 team|solo|true|false|1|0；可选 teamSize）

    修改模式与队伍大小；会清空在线玩家并重置 ID
    同步写回 server.ini

## 许可证
本项目采用 MIT 许可证，详见 LICENSE 文件。
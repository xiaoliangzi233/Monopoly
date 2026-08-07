# 霓城大亨 / Neon Tycoon

一款使用 C++20 与 Qt 6 开发的原创 2.5D 城市经营派对游戏。地图、建筑、人物、道路和装饰物全部由程序化几何绘制，不依赖外部美术贴图。

## 当前可玩内容

- 48 格等距霓虹城市地图、24 块地产和 8 个街区。
- 2–6 名玩家，本地热座与三级 AI 混合对局。
- 掷骰、有限重掷、买地、三级建设、租金、税费、抵押/赎回和限轮计分。
- 6 名角色的主动技能与被动能力、36 张策略卡、30 个个人事件、18 个任务和 8 种可改变租金/建造价格/玩家状态的城市脉冲。
- 高性能 Qt Scene Graph 场景：建筑立面、屋顶、窗户、招牌、天线、道路、路灯、树木、交通站和人物均由代码绘制。
- 世界锚点与深度排序；当前角色始终有顶层轮廓和光环，所在建筑自动淡化。
- 折叠侧栏、悬浮信息、上下文操作栏、默认最大化窗口和高 DPI UI。
- 房主权威 TCP 联机、CBOR 协议、UDP 局域网发现、状态哈希、快照同步、唯一席位授权和断线 AI 托管。
- SHA-256 原子存档、快速恢复、独立启动器、GitHub/Gitee 双更新源和 Ed25519 签名校验。

当前版本是可安装、可玩和可联机的 Alpha。交易协商、限时拍卖、两个真实路线分支、全量卡牌独立效果、更完整的断线恢复 UI、更新器版本目录原子切换/自动回滚以及最终数值平衡仍是正式版的后续工作。

## 本地构建

要求：CMake 3.24+、Ninja、Qt 6.4+（正式 CI 使用 Qt 6.8 LTS）、支持 C++20 的编译器。Windows 推荐 MSVC 2022；也可使用匹配同一 MSYS2 环境的 MinGW Qt。

```powershell
cmake -S . -B build/release -G Ninja `
  -DCMAKE_PREFIX_PATH=C:/Qt/6.8.3/msvc2022_64 `
  -DCMAKE_BUILD_TYPE=Release
cmake --build build/release
ctest --test-dir build/release --output-on-failure
```

本机 MSYS2 示例：

```powershell
$env:PATH="C:\msys64\mingw64\bin;$env:PATH"
cmake -S . -B build/msys -G Ninja -DCMAKE_PREFIX_PATH=C:/msys64/mingw64
cmake --build build/msys
ctest --test-dir build/msys --output-on-failure
```

游戏程序为 `NeonTycoon.exe`，启动器为 `NeonTycoonLauncher.exe`。使用 CPack 生成初始安装器：

```powershell
cpack --config build/release/CPackConfig.cmake -G NSIS
```

## 联机

房主在“联机”面板配置总人数和 AI 数并创建房间。局域网或 Radmin 玩家填写房主 IPv4，默认 TCP/UDP 端口为 `29450/29451`。防火墙需允许游戏访问专用网络。

服务端只接受当前连接被分配席位的命令，并再次校验对局、玩家、阶段、余额和产权。远程玩家掉线 60 秒后由 AI 接管；保留的玩家身份允许重连恢复。

## 发布与更新安全

发布流水线需要以下 GitHub Repository Secrets：

- `GITEE_TOKEN`：仅授予目标 Gitee 仓库和 Release 所需最小权限。
- `UPDATE_SIGNING_KEY`：Ed25519 私钥的 Base64 编码，仅供 CI 签署更新清单。

以及 Repository Variable `GITEE_REPOSITORY`，格式为 `owner/repository`。令牌和私钥不得写入源码、清单、客户端或聊天记录。

配置 CMake 缓存项 `NEON_UPDATE_PUBLIC_KEY_HEX`、`NEON_GITHUB_MANIFEST_URL` 和 `NEON_GITEE_MANIFEST_URL` 后，生产启动器才会接受远程更新。清单 URL 指向各源的 `stable.json`；当玩家切换到测试通道时，启动器会自动请求同目录的 `beta.json`。未配置公钥时启动器采用失败关闭策略，只允许启动已安装版本。

## 质量检查

默认测试覆盖地图内容、建筑/道路占地、世界锚点、命令幂等、买地升级、卡牌/技能/抵押、存档校验、协议帧限制和 AI 模拟。联机集成测试会真实建立本机 TCP 房间，验证座位授权、越权拒绝、快照序号与状态哈希。发布 CI 设置 `NEON_SIMULATION_GAMES=10000`，验证一万局不会死循环或产生负现金异常。

项目源码使用 MIT 许可证。发布 Qt 动态库时须同时提供 Qt LGPL 许可证和对应 notices。

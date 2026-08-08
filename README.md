# 盛世百业

《盛世百业》是一款使用 C++20 与 Qt Quick 开发的原创现代国潮多人经营游戏。玩家选择六名原创城市产业主理人之一，在新中式都市的八个街区经营产业、参与竞价与交易，并以资产、品牌、创新、宜居四维繁荣决出胜负。

公开名称自 v0.2.0 起改为《盛世百业》。为保证 v0.1.0 玩家能够原地升级，内部程序名仍为 `NeonTycoon.exe`，启动器仍为 `NeonTycoonLauncher.exe`，应用数据目录和安装目录标识也保持不变。

## v0.2.0 内容

- 4096×3072 正交俯视世界、64 个相连节点、八个现代国潮街区和分支路线。
- 32 处三级现代产业，以及城市事件、交通枢纽、产业协作、公共项目、创新委托、运营费用和潮流节节点。
- 空白处左键或中键拖拽、滚轮光标中心缩放、WASD/方向键平移、回合自动聚焦、移动跟随和可折叠舆图。
- 2–6 人本地热座、AI 或局域网/Radmin IPv4 联机；协议 v2 使用房主权威随机数、幂等命令、事件序号、状态哈希和快照重同步。
- 骰子动画、权威逐格移动、路线选择、重掷、购买、三级建设、抵押、出售、30 秒竞价、交易、角色主动技能、城市脉冲和四维终局计分。
- 48 张策略卡、36 个城市事件、24 个项目任务和 12 个城市脉冲。
- 全部道路、现代建筑、玻璃幕墙、绿化、桥梁、地标和人物均由代码生成，不依赖来源不明的外部游戏贴图。
- 程序化五声音阶音乐与操作音效；音乐、效果音和静音设置独立持久化。缺少 Qt Multimedia 时仍可构建静音版本。
- v2 原子存档与 SHA-256 校验。v0.1.0 存档不会误迁移，而会安全归档到 `saves/archive-v1`。

默认标准局为 120 回合，也可选择 90 或 150 回合。仅剩一名未破产玩家时提前结束；否则总繁荣满分 400，同分依次比较现金、未抵押产业数和座位顺序。

## 构建与测试

要求 CMake 3.24+、Ninja、Qt 6.8.3（Core、Gui、Network、Qml、Quick、QuickControls2、ShaderTools、Multimedia）和支持 C++20 的编译器。正式 CI 使用 MSVC 2022；本地也可使用匹配同一 MSYS2 环境的 MinGW Qt。

```powershell
cmake -S . -B build/release -G Ninja `
  -DCMAKE_PREFIX_PATH=C:/Qt/6.8.3/msvc2022_64 `
  -DCMAKE_BUILD_TYPE=Release
cmake --build build/release
ctest --test-dir build/release --output-on-failure
```

执行完整 10,000 局模拟：

```powershell
$env:NEON_SIMULATION_GAMES = '10000'
ctest --test-dir build/release --output-on-failure
```

## 安装包与启动入口

生成 NSIS 安装包：

```powershell
cpack --config build/release/CPackConfig.cmake -G NSIS -B build/installer
```

v0.2.0 文件名为 `ShengshiBaiye-0.2.0-win64.exe`。安装器会创建桌面和开始菜单“盛世百业”快捷方式，目标为安装目录中的 `bin\NeonTycoonLauncher.exe`。玩家应从这个启动器进入游戏，以便先检查 GitHub/Gitee 更新；也可直接运行同目录的 `NeonTycoon.exe`。

## 联机

房主在“联机游戏”面板设置人数、AI 与回合数并创建房间。局域网或 Radmin 玩家填写房主 IPv4，默认 TCP 游戏端口为 `29450`，UDP 发现端口为 `29451`。防火墙需要允许游戏访问专用网络。协议 v1 客户端会收到明确的不兼容提示，不能加入 v2 房间。

房主只接受连接所获座位的合法意图，并复核回合、阶段、余额、产权和超时。客户端按序应用事件并比对状态哈希，不一致时自动获取快照。普通玩家掉线后可由 AI 托管，并可按原身份重新接管。

## 一键发布

仓库根目录的 `publish_release.bat` 默认使用本机发布，不消耗 GitHub Actions：它会构建并运行 10,000 局测试、生成 NSIS 安装包、创建 GitHub/Gitee Release、使用原 Ed25519 私钥签名，并推送双源更新清单。

```powershell
# 只检查本机构建与密钥，不产生标签或远端变更
.\scripts\publish_release_local.ps1 -Version 0.2.0 -SkipTests -DryRun -Yes

# 发布当前已经写入 VERSION 的 v0.2.0
.\publish_release.bat -Version 0.2.0

# 如需继续使用 GitHub Actions 发布，可显式运行旧流程
.\scripts\publish_release.ps1 -Version 0.2.0 -Yes
```

本机发布需要仅存在于当前 PowerShell 会话中的环境变量：

- `UPDATE_SIGNING_KEY`：与现有启动器公钥匹配的 Ed25519 私钥 Base64。
- `GITEE_TOKEN`：已轮换的新最小权限令牌。

首次使用前安装签名和 Gitee API 依赖：`python -m pip install pynacl requests`。
脚本会先验证私钥是否匹配生产公钥，不匹配时不会创建标签。不要将私钥或令牌写入批处理、仓库或日志。GitHub Actions 备用流程则需要 Repository Secrets：

- `UPDATE_SIGNING_KEY`：原有 Ed25519 私钥的 Base64，仅供 CI 签署清单。
- `GITEE_TOKEN`：撤销曾在聊天或日志中暴露的令牌后创建的新最小权限令牌。

并需要 Repository Variables：`GITEE_REPOSITORY`、`PRIMARY_MANIFEST_URL`、`GITEE_MANIFEST_URL` 和 `UPDATE_PUBLIC_KEY_HEX`。令牌和私钥不得写入代码、配置、客户端或聊天记录。GitHub Actions 账户存在 billing lock 时必须先解除，否则标签会推送但构建任务无法启动。

更新清单强制校验 SHA-256 和 Ed25519 签名，最低启动器版本保持 `0.1.0`。GitHub 源失败时会切换到 Gitee；公钥未配置或签名无效时，启动器拒绝安装远程包，但仍允许启动已安装版本。

## 许可

项目源码使用 MIT 许可。Qt 以 LGPL 动态链接方式发布，安装包同时包含相应许可与 notices。角色、规则文本与现代国潮视觉均为原创设计。

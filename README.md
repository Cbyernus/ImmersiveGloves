# ImmersiveGloves

我想分享如何使用两个Tundra Tracker、两个接口板和12个惯性传感器制作VR手套。

https://github.com/ras-marques/ImmersiveGloves/assets/6479742/4af1af61-4261-44ce-aa3d-e45de0df7709

请查看 YouTube 上的完整演示: https://www.youtube.com/watch?v=5OSYiYDkOE8

手部追踪功能通过在每个手指上安装一个惯性测量单元（IMU）来实现，支持手指的卷曲和展开。当拇指触碰到食指时，摇杆功能会被启用；而通过拇指触碰到中指、无名指和小指，分别可以激活 B、A 和系统按钮。

## 整體儲存庫狀態

代碼不完整。
主板和IMU板的文件已更新。
目前只有右手手套的代码。
3D打印文件需要进行调整。

## 所需设备和技能

 - 裸 PCB https://github.com/ras-marques/ImmersiveGloves/tree/main/CustomPCBs/ReadyToTest
 - 用于填充 PCB 的组件 https://github.com/ras-marques/ImmersiveGloves/blob/main/CustomPCBs/cost_analysis.ods
 - 焊膏
 - 热风枪或热板
 - 烙铁
 - 热熔胶枪或超级胶水
 - 3D 打印机
 - 预压接 JST 1.0mm 硅胶线套件 https://www.amazon.es/gp/product/B07PDQKHJ2/ref=ppx_yo_dt_b_asin_title_o00_s00?ie=UTF8&psc=1

在这个项目中，最难的部分是制作电路板。目前我不提供电路板，所以您需要从其他地方订购。

这些电路板是四层的，具有细间距组件。在家制作 PCB 是不切实际的，但您可以从 JLCPCB、PCBWAY 等地方订购。

我使用模板手工组装了手套上的组件，使用焊膏和热风枪进行回流焊。使用热板效果更佳。

如果您没有 3D 打印机，可以从其他地方订购模型或请朋友帮忙。

目前所有组件都粘在手套布料上，您可以使用热熔胶或超级胶水。

预压接的 JST 线是使组装过程更容易的必要材料。

## 成本分析

根据组件和运输成本，假设您已经拥有所有工具（如 3D 打印机和焊接工具），每双手套的实际成本大约在 250€ 左右。

完整的BOM可以參考https://github.com/ras-marques/ImmersiveGloves/blob/main/CustomPCBs/cost_analysis.ods ，但總結一下目前的成本如下：
- 200€：制造足够的电路板以供 50 双手套，成本为 4€/双。
- 170€：为一双手套采购足够的组件和一些备用件的成本。
- 6500€：为 50 双手套采购足够的组件的成本，计算为 130€/双，乘以 50

## 追蹤方式

使用 Tundra Tracker 为这些手套提供基础的原因是，使用 Tundra Tracker 和扩展板可以避免处理电池问题，因为 Tundra Tracker 可以为所有设备供电。此外，追踪器还负责将手套的数据无线传输到计算机。两个 RP2040 负责从六个惯性传感器获取数据、处理数据并将其传递给追踪器。

该项目面向已经拥有激光追踪 VR 设置并使用基站的用户，理想情况下是使用 Tundra Trackers 进行全身追踪的用户。不过，如果您还没有 Tundra Trackers，有以下一些选项：

- 400€：2 套独立 Tundra Tracker 和 SW1 dongle（需要 2 个 USB 端口） Tundra Tracker + Tundra SW1 Dongle
- 459€：3 套 Tundra Tracker 套装（需要 1 个 USB 端口） Tundra Tracker x3 Bundle
- 609€：4 套 Tundra Tracker 套装（需要 1 个 USB 端口） Tundra Tracker x4 Bundle
此外，如果您没有基站，Valve 的基站售价为 159€，最好是购买 2 个。

## 這個手套的最終目標

- Tundra Tracker 非常出色，它是可用的最小追踪器，还支持将传感器数据发送到计算机。这是这些手套的终极追踪器，但我希望让这些设备对每个人都更加可及，因此我想支持更多的追踪方法，从使用您现有的控制器开始，就像 LucidVR 一样。这可能意味着需要重新设计主板，以便能够使用外部电池供电。
- 我最初是从 Slime VR 的 BNO085 板开始这个项目的，之后设计了一个尽可能小的定制 BNO085。我甚至没有包含 JST 连接器，电线直接焊接到板上。不过，为了便于组装，我考虑在下一个 IMU 板中包含连接器。我还想探索使用更便宜的 IMU 芯片，如 BMI270，带或不带磁力计。
- 我的手套将手指灵活性和低调设计作为首要考虑，因此除非绝对必要，否则我没有计划实现物理按钮或力反馈，但如果足够小，振动触觉是可能的。我鼓励您这样做，并分享您自己的手套版本！

## 手套製作步驟：

### 3D 列印追蹤器的安裝點

- 该仓库的 3DPrintingFiles 文件夹中提供了 3D 打印文件。这不是一个通用的安装点。我使用 3D 打印的螺丝插入矩阵测量了我手背的曲率，因此它完美贴合。未来我会展示我的过程，以便您可以在需要时进行相同的操作。

### 準備 Tundra Tracker
- 从该仓库下载文件。
- 打开文件夹 {Steam}\steamapps\common\SteamVR\tools\lighthouse\bin\win32 ({Steam} 可能是 C:\Program Files (x86)\Steam，但具体情况可能不同）。
- 将此仓库中的 ImmersiveGlovesLeft.json 和 ImmersiveGlovesRight.json 复制到该文件夹中。
- 从计算机上断开所有 VR 设备，包括接收追踪器数据的接收器。使用 USB 电缆仅连接一个 Tundra Tracker 到计算机。
- 在地址栏中输入 cmd，以在 {Steam}\steamapps\common\SteamVR\tools\lighthouse\bin\win32 打开命令提示符。
- 在命令提示符中输入 lighthouse_console.exe 并按回车。
- 您应该只看到一个设备连接，如果有更多设备，请确保断开它们，这一点 很重要。输入 downloadconfig backup.json 以存储 Tundra Tracker 配置文件的备份。
- 使用命令行中的 uploadconfig ImmersiveGlovesRight.json 将 ImmersiveGlovesRight.json 配置存储到追踪器中。这将是您右手手套的追踪器。
- 断开该追踪器并连接另一个追踪器。
- 使用命令行中的 uploadconfig ImmersiveGlovesLeft.json 将 ImmersiveGlovesLeft.json 配置存储到追踪器中。这将是您左手手套的追踪器。
- 最后，通过将仓库中的 driverProject/immersive_gloves_controller/build/immersive_gloves_controller 文件夹复制到 {Steam}\steamapps\common\SteamVR\drivers\ 来安装 ImmersiveGloves 驱动程序。

### 刷RP2040 板 (文件尚未可用!)
- 从这里下载最新的稳定版本的 CircuitPython：CircuitPython 下载。
- 将扩展板连接到 Tundra Tracker，然后使用配套的 USB 板通过 USB1 连接到 PC。
- 在 Tracker 关闭的状态下，按下 USB 板上的按钮，同时启动 Tundra Tracker，这样可以将 RP2040 设置为扩展板的引导加载程序模式。
- 在您的 PC 上应该会出现一个窗口，显示 RP2040 模拟的新驱动器。将您之前下载的 CircuitPython 文件复制到此文件夹中。RP2040 会自动重启。
- 像之前一样，您的 PC 上应该会出现另一个窗口，显示另一个驱动器，但结构不同。这是您放置处理传感器数据并将其发送到计算机的程序的地方。只需将 rp2040Firmware/usb1 文件夹中的内容复制到此新文件夹的根目录。
- 对第二个 RP2040 MCU 重复上述步骤，使用 USB2 和 rp2040Firmware/usb2 中的文件。

### 驱动程序
- 我们使用 OpenGloves 驱动程序，以便轻松获取与所有支持 Index 控制器的应用程序的输入，这几乎包括所有应用程序。请从 Steam 下载：[OpenGloves](https://store.steampowered.com/app/1574050/OpenGloves)
- 将此仓库中 driverProject/immersive_gloves_controller/build/immersive_gloves_controller 的内容复制到 C:\Program Files (x86)\Steam\steamapps\common\SteamVR\drivers 或任何安裝了 SteamVR 應用程式的地方。
- 确保在 SteamVR 设置中的启动/关闭选项卡中启用 OpenGloves 和 immersive_gloves_controller 附加组件，点击“管理 SteamVR 附加组件”，这两个组件应当处于开启状态。每当 SteamVR 由于错误关闭时，可能会导致其中一个或两个组件被关闭，因此如果有任何问题，请检查这一点。
- 您应该在 OpenGloves 配置面板上校准旋转和位置偏移。我是手动进行校准的，因为我暂时还没有手套上的按钮来触发校准界面。未来可能会提供预校准文件及使用说明。

## 以下是使这个项目成为可能的重要资源：
 
Tundra Labs 有一個儲存庫，其中包含一些有關如何使用其開發板的文檔 https://github.com/tundra-labs/rp2040_examples

CircuitPython 讓我可以輕鬆地讓imu工作，而無需從頭開始編寫所有內容，您可以在此處了解更多信息 https://learn.adafruit.com/adafruit-9-dof-orientation-imu-fusion-breakout-bno085/python-circuitpython

最後，Functional 的 Open VR 驅動程式教學很好地介紹了 OpenVR 輸入的工作原理 https://www.youtube.com/watch?v=LzEIOBnbC8k

OpenVR 儲存庫有大量文檔，需要一段時間才能找出對該專案重要的部分，但這是值得的 https://github.com/ValveSoftware/openvr

[danwillm](https://github.com/danwillm) 透過為我指明正確的方向，告訴我使用哪些技術來與追蹤器和 openvr 對話，幫助我開始了這件事。

## 貢獻工作流程

以下是我建议您如何向该项目提议更改的步骤：

1. [Fork this project][fork] 到您的账户
2. [Create a branch][branch] 用于您打算进行的更改
3. 对您的 fork 进行更改。
4. [Send a pull request][pr] 從您 fork 的分支到我們的 `main` 分支。

使用基於網頁的介面進行更改也是可以的，這將自動為您 fork 項目並提示您發送拉取請求。這樣可以簡化整個流程。

[fork]: https://help.github.com/articles/fork-a-repo/
[branch]: https://help.github.com/articles/creating-and-deleting-branches-within-your-repository
[pr]: https://help.github.com/articles/using-pull-requests/

### 其他語言
[English](https://github.com/ras-marques/ImmersiveGloves) |[中文](https://github.com/Cbyernus/ImmersiveGloves/blob/main/README.md)

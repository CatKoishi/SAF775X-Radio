<!-- markdownlint-disable MD026 MD028 MD033 MD041 -->

<div align="center">
  <a href="https://github.com/CatKoishi" target="_blank">
    <img width="160" src="https://avatars.githubusercontent.com/u/74466081?v=4" alt="logo">
  </a>

<h1><ruby>SAF775X Radio<rp>(</rp><rt>DIRANA3 收音机</rt><rp>)</ruby></h1>

</div>

![主图](/Asset/main_pic.png "收音机实物图")

## 📖 简介

基于恩智浦Dirana3系列的多波段收音机，具体型号为SAF775X，支持双天线接收，可启用多种高级收音算法，由于使用的是演示密钥，所以开机3小时后会自动复位

此密钥不涉及对SAF775X芯片的破解，没有商用价值，与 NXP 的企业权益无冲突，演示密钥与部分程序来源于[rayc345/WTCRC775X](https://github.com/rayc345/WTCRC775X)

#### 🔗 项目链接

 ➡️ [软件开源 GitHub](https://github.com/CatKoishi/SAF775X-Radio)

 ➡️ [硬件开源 OSHWHub](https://oshwhub.com/imhhh/saf775x-radio)

## 📦 功能

略，请看视频（没录）

## 📦 ToDo

- [ ] 收音动态高切调节
- [ ] 音频静态/动态响度
- [ ] 音频正弦波/噪声发生器UI
- [ ] ALE 自动响度均衡
- [x] UltraBass 自适应虚拟低音
- [ ] 音频滤波器组驱动与UI
- [x] 扬声器启用自动单声道
- [ ] 主界面EQ/Tone
- [ ] 低功耗与关机
- [ ] 针对AM噪声的pEQ
- [ ] 利用通用滤波器，延迟线的伪立体声

---

## 📝 更新记录

### 20260831-2.9

- 增加 44.1 kHz 与 48 kHz 采样率切换

### 20260527-2.8

- 新增 Coax Output 与 Host I2S0 Out 开关
- 启用 Coax Output 或 Host I2S0 Out 时自动关闭 AMP 电源，避免喇叭继续输出
- 优化同轴/Host I2S0 输出切换时的喇叭输出控制
- 调整存台上限：LW 20 台，MW 50 台，FM 150 台，SW 200 台
- 修复 100 台以上存台时的索引与主界面显示问题
- 修复 ATS Threshold 设置无法保存的问题

### 20260519-2.7

- 增加电台删除功能 [设置-Search-Manage Memory]

### 20260417-2.6

- 修复错误的 SFUD 擦除参数
- CRC 相同的实时信息不会写入 Flash

### 20260101-2.5

- 修复设置保存在某些情况下的错误
- 修复错误的静音设置
- 修复启动过程中某些配置未设置

### 20251121-2.4

- 将 FlashDB 更换为 LittleFS, 理论上储存更加稳健

### 20250707-2.3.1

- 开机按住左编码器会强制初始化设置
- 限制对比度调节范围

### 20250408-2.3

- Flash 驱动改为 SFUD，理论上兼容所有支持 JEDEC 标准的 Flash (容量 > 2MByte)
- 修复某些情况下 Device 下的设置不保存的问题

### 20250321-2.2.1

- 设置版本不匹配时重置数据库，减小报错

### 20250320-2.2

- 增加 RDS RadioText 显示
- 非 EmiFree 模式下存台信息由 CH.05 改为 CH.05/14
- 将存台序号最小值改为 01
- 修复在非 EmiFree 模式下 RDS PI 码侵占存台序号的问题
- 修复进退菜单后主界面的 RDS PI/PS 信息丢失
- 修复错误读取版本信息

### 20250311-2.1

- FMSI 立体声等级可调
- 使用扬声器时可设置自动输出单声道（提高信噪比）

### 20250308-2.0

- FM切换频率无静音
- 所有设置使用FlashDB存储，支持Flash磨损均衡
- 无操作1分钟后存储实时信息
- 增加主菜单图标

<details>
<summary>历史记录（点击展开）</summary>

### 20240802-1.7a

- 超频SPI通讯速率到30MH

### 20240801-1.7

- 重构主界面UI
- 新增Audio目录图标
- 新增RDS PI PS信息展示

### 20240723-1.6

- 重构LCD DMA刷屏函数与GUI函数，减小屏幕通信干扰
- 修复开机屏幕不打开无限背光的Bug

### 20231105-1.5

- 增加DAC音频增益3dB
- 略微降低屏幕通信干扰
- VU表自衰减

### 20231007-1.4

- 新增About图标

### 20230924(ER)

- 修复flash排列错误

### 20230921-1.4

- 修复开机LNA不启用的Bug
- 时钟源更改为内部RC振荡器
- 版本标记修改
- 增加UltraBass功能（KeyCode）
- 增加GSA，VU流畅度

### 20230913-1.3

- 修复切换波段静音解除的问题
- 修复775x复位后静音解除的问题
- 增加NoiseBlanker, SoftMute设置
- 设置带宽增加指示

### 20230912-1.2

- 修改音量线性度
- 修改音量步进
- 优化GSA,VU,RSSI延迟
- 加快VU表衰落速度

### 20230911-1.1

- 降低屏幕通信干扰

</details>

<!-- markdownlint-disable MD026 MD028 MD033 MD041 -->

<div align="center">
  <a href="https://github.com/CatKoishi" target="_blank">
    <img width="160" src="https://avatars.githubusercontent.com/u/74466081?v=4" alt="logo">
  </a>

<h1><ruby>SAF775X Radio<rp>(</rp><rt>DIRANA3 收音机</rt><rp>)</ruby></h1>

</div>

## 📦 功能

略，请看视频（没录）

## 📦 ToDo

- [ ] 收音动态高切调节
- [ ] 音频静态/动态响度
- [ ] 音频正弦波/噪声发生器UI
- [ ] ALE 自动响度均衡
- [x] UltraBass 自适应虚拟低音
- [ ] 音频滤波器组驱动与UI
- [ ] 扬声器启用自动单声道
- [ ] 主界面EQ/Tone
- [ ] 低功耗与关机
- [ ] 针对AM噪声的pEQ
- [ ] 利用通用滤波器，延迟线的伪立体声

---

## 📝 更新记录

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

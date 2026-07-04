# RTMPKit - 专业 RTMP 流媒体工具集

湖南云立方智能科技有限公司是一家专注于人工智能领域的科技公司，在图像识别和智能硬件领域拥有多年的创业经验与产品积累。

**RTMPKit** 是一套基于著名的 **rtmpdump** 项目的 RTMP 流媒体开源工具集。本项目的核心贡献是自主开发的 **rtmppub** 高性能 RTMP 推流器，其他工具（rtmpdump、rtmpgw、rtmpsrv、rtmpsuck）均来自原始 rtmpdump 项目，未做修改。

## � 鸣谢

本项目基于 **rtmpdump** 开源项目进行开发，感谢原始作者和贡献者的辛勤工作：

- **rtmpdump** - 原始 RTMP 流媒体工具集
  - (C) 2009 Andrej Stepanchuk
  - (C) 2009-2011 Howard Chu
  - 项目地址: http://rtmpdump.mplayerhq.hu/

本项目基于 rtmpdump 的优秀基础，除自主开发的 rtmppub 推流器外，其他工具均保持原始 rtmpdump 的功能和代码，未做修改。我们尊重原始版权，并在此基础上提供更完善的整体解决方案。

## 🚀 核心功能

### 🆕 rtmppub - RTMP 流媒体推流器（自主开发）
**高性能 RTMP 推流工具**，由湖南云立方智能科技有限公司自主开发，支持将本地 FLV 文件或 H264 文件推送到 RTMP 服务器（如 SRS、Nginx-RTMP、Wowza 等）。

**主要特性：**
- ✅ 支持 FLV 文件推流
- ✅ 支持 H264 原始文件推流
- ✅ 自动解析 SPS/PPS 获取视频信息
- ✅ 精确时间戳控制，确保流畅播放
- ✅ 详细日志输出，便于调试

**使用示例：**
```bash
# 推流 FLV 文件
rtmppub -i input.flv -r rtmp://127.0.0.1:1935/live/test

# 推流 H264 文件
rtmppub -i input.h264 -r rtmp://127.0.0.1:1935/live/test
```

---

### 上游项目工具（来自 rtmpdump）

以下工具均来自原始 rtmpdump 项目，本项目未做修改：

#### rtmpdump - RTMP 流媒体下载器
**专业级 RTMP 流媒体下载工具**，支持从 RTMP 服务器抓取流媒体内容并保存为 FLV 文件。

**主要特性：**
- ✅ 支持 RTMP、RTMPE、RTMPS 等多种协议
- ✅ 支持直播流录制和点播下载
- ✅ 支持断点续传
- ✅ 支持 SWF Verification 自动验证

**使用示例：**
```bash
# 下载 RTMP 流
rtmpdump -r rtmp://server/live/stream -o output.flv

# 下载直播流
rtmpdump -r rtmp://server/live/stream -v -o live.flv

# 从指定时间开始下载
rtmpdump -r rtmp://server/vod/video -A 60 -o clip.flv
```

#### rtmpgw - RTMP 转 HTTP 网关
**RTMP 到 HTTP 的转换网关**，允许通过 HTTP 协议访问 RTMP 流媒体内容。

**主要特性：**
- ✅ 将 RTMP 流转换为 HTTP 响应
- ✅ 支持 URL 参数配置
- ✅ 支持直播和点播
- ✅ 可配置监听端口和缓冲区

**使用示例：**
```bash
# 启动 HTTP 网关（默认端口 80）
rtmpgw -r rtmp://server/live/stream

# 自定义监听端口
rtmpgw -r rtmp://server/live/stream -g 8080
```

#### rtmpsrv - RTMP 测试服务器
**轻量级 RTMP 服务器**，用于测试和调试 RTMP 客户端连接。

**主要特性：**
- ✅ 监听 RTMP 连接请求
- ✅ 自动解析客户端连接参数
- ✅ 自动调用 rtmpdump 进行录制
- ✅ 支持端口配置

**使用示例：**
```bash
# 启动测试服务器（默认端口 1935）
rtmpsrv

# 查看调试信息
rtmpsrv -z
```

#### rtmpsuck - RTMP 透明代理
**RTMP 透明代理服务器**，用于捕获和记录 RTMP 通信流量。

**主要特性：**
- ✅ 透明代理 RTMP 会话
- ✅ 解密并记录流媒体数据
- ✅ 支持客户端和服务器双向通信
- ✅ 自动保存音视频数据到文件

**使用示例：**
```bash
# 启动透明代理
rtmpsuck

# 启用调试日志
rtmpsuck -z
```

## 📦 编译安装

### Windows 平台
```bash
# 编译所有工具
make SYS=mingw

# 单独编译推流器
make SYS=mingw rtmppub
```

### Linux/Unix 平台
```bash
# 编译所有工具
make SYS=posix

# 单独编译推流器
make SYS=posix rtmppub
```

### macOS 平台
```bash
# 编译所有工具
make SYS=darwin

# 单独编译推流器
make SYS=darwin rtmppub
```

### 无加密模式编译（推荐）
```bash
# 禁用加密支持，减小体积
make SYS=mingw CRYPTO=
```

## 🔧 技术特性

- **协议支持**: RTMP, RTMPE, RTMPS, RTMPT, RTMPTE
- **编码支持**: H.264, AAC, MP3, VP6, Speex
- **跨平台**: Windows, Linux, macOS
- **开源协议**: GPLv2 (工具), LGPLv2.1 (librtmp 库)

## 📖 详细文档

- [RTMPDump 手册](rtmpdump.1.html)
- [RTMPPub 推流器手册](rtmppub.html)

## 🤝 联系方式

**湖南云立方智能科技有限公司**

- 🌐 官网: https://www.turingdance.com
- 💬 微信: huwinlion
- 📧 邮箱: 271151388@qq.com

## 🌟 关于我们

湖南云立方智能科技有限公司是一家专注于人工智能领域的科技公司，在图像识别和智能硬件领域拥有多年的创业经验与产品积累。我们致力于将人工智能技术与硬件产品深度融合，打造创新的智能解决方案。

- **图像识别领域**: 深耕多年，积累了丰富的计算机视觉算法研发经验，在目标检测、图像分类、人脸识别等领域拥有成熟的技术方案和产品落地能力。
- **智能硬件领域**: 具备从硬件设计、嵌入式开发到云端服务的全链路能力，打造了多款具有自主知识产权的智能硬件产品。
- **流媒体技术**: 作为我们产品线的重要组成部分，自主开发的 **rtmppub** 推流器展示了我们在音视频编解码、实时传输等方面的技术实力，为流媒体应用提供稳定可靠的技术支撑。

我们的技术团队由资深工程师和行业专家组成，始终秉持创新驱动的理念，致力于为客户提供领先的人工智能解决方案。

## 📄 许可证

RTMPKit 工具集基于 rtmpdump 项目开发，使用 GPLv2 许可证发布，librtmp 库使用 LGPLv2.1 许可证发布。详情请参阅 COPYING 文件。

---

**关键词**: RTMP推流, RTMP下载, RTMP服务器, SRS推流, 直播推流, 流媒体, H264推流, FLV推流, rtmpdump, rtmppub, RTMPKit
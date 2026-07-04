# RTMPKit - 专业 RTMP 流媒体工具集

**RTMPKit** 是一套功能强大的 RTMP 流媒体开源工具集，基于著名的 **rtmpdump** 项目进行优化和扩展，支持 RTMP 协议的完整功能，包括流媒体下载、推流、服务器和代理等核心功能。由湖南云立方智能科技有限公司维护和优化，致力于为开发者提供最稳定可靠的 RTMP 解决方案。

## � 鸣谢

本项目基于 **rtmpdump** 开源项目进行开发，感谢原始作者和贡献者的辛勤工作：

- **rtmpdump** - 原始 RTMP 流媒体工具集
  - (C) 2009 Andrej Stepanchuk
  - (C) 2009-2011 Howard Chu
  - 项目地址: http://rtmpdump.mplayerhq.hu/

没有 rtmpdump 的优秀基础，就没有 RTMPKit 的今天。我们在尊重原始版权的基础上，添加了新的功能和优化，使其更加适合现代流媒体应用场景。

## �🚀 核心功能

### rtmpdump - RTMP 流媒体下载器
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

### rtmppub - RTMP 流媒体推流器
**高性能 RTMP 推流工具**，支持将本地 FLV 文件或 H264 文件推送到 RTMP 服务器（如 SRS、Nginx-RTMP、Wowza 等）。

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

### rtmpgw - RTMP 转 HTTP 网关
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

### rtmpsrv - RTMP 测试服务器
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

### rtmpsuck - RTMP 透明代理
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

湖南云立方智能科技有限公司专注于音视频流媒体技术研发，提供专业的 RTMP 推流、直播、点播解决方案。我们的技术团队拥有丰富的流媒体开发经验，致力于为客户提供稳定、高效、可靠的技术服务。

## 📄 许可证

RTMPKit 工具集基于 rtmpdump 项目开发，使用 GPLv2 许可证发布，librtmp 库使用 LGPLv2.1 许可证发布。详情请参阅 COPYING 文件。

---

**关键词**: RTMP推流, RTMP下载, RTMP服务器, SRS推流, 直播推流, 流媒体, H264推流, FLV推流, rtmpdump, rtmppub, RTMPKit
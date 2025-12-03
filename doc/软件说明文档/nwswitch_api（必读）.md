 # 丰柯网络交换机 API 说明文档

> 版本：v3.0.0 | 作者：leiwei | 日期：2025-07-25

## 📖 简介

丰柯网络交换机API提供高性能的交换机通信接口，支持命令传输、结果接收、连接管理和网络配置。

**主要特点**：安全连接、命令执行、超时控制、多设备支持、跨平台兼容、网络配置管理

## 📁 目录结构

```
nwswitch_api/
├── README.pdf                 # 说明文档
├── NWSwitchApiReleaseNote.txt # 版本变更记录
├── NWSwitchClient.py          # Python API封装
├── NWSwitchClientTest.py      # Python测试工具
├── include/nwswitchapi.h      # C/C++ 头文件
└── lib/
    ├── Winx64/                # 64位库文件
    │   └── nwswitch_api.dll
    │   └── nwswitch_api.lib
    └── Winx86/                # 32位库文件
        └── nwswitch_api.dll
        └── nwswitch_api.lib
```

注意：运行时需要拷贝依赖库，如Qt/SSH库

## 🚀 快速开始

### Python测试（推荐新手）

1. **启动测试工具**

   ```bash
   python NWSwitchClientTest.py
   # 64位python环境启动测试：
   py -3-64 NWSwitchClientTest.py
   # 32位python环境启动测试：
   py -3-32 NWSwitchClientTest.py
   ```

2. **基本操作**

   ```bash
   >>> --connect 192.168.1.120/1     # 连接交换机
   >>> --send "ping 192.168.1.108"   # 发送命令
   >>> --autoRead on                 # 开启自动读取
   >>> --read 5000                   # 手动读取结果
   >>> --disconnect                  # 断开连接
   >>> --help                        # 查看帮助
   >>> --exit                        # 退出
   ```

3. **网络配置管理**

   ```bash
   >>> --configNet 192.168.1.120 nwswitch_config.json   # 配置网络
   >>> --syncNet 192.168.1.120 nwswitch_backup.json      # 同步配置
   >>> --autoStart 192.168.1.120                                # 设置自启动
   ```

### C/C++ 开发

1. **包含头文件**

   ```cpp
   #include "nwswitchapi.h"
   #pragma comment(lib, "nwswitch_api.lib")
   ```

2. **基本使用**

   ```cpp
   // 连接交换机
   int instance = NWSwitchConnect("192.168.1.120/1");
   if (instance > 0) {
       // 发送命令
       NWSwitchSendCmd(instance, "ping 192.168.1.108");
   
       // 接收结果
       char buffer[4096] = {0};
       NWSwitchReceiveCmdResult(instance, buffer, sizeof(buffer), 5000);
   
       // 关闭连接
       NWSwitchClose(instance);
   } 
   ```

3. **网络配置管理**

   ```cpp
   // 配置交换机网络
   NWSwitchConfigureNetwork("192.168.1.120", "nwswitch_config.json");
   
   // 同步网络配置
   NWSwitchSyncNetwork("192.168.1.120", "nwswitch_backup.json");
   
   // 设置自启动
   NWSwitchSetupAutostart("192.168.1.120");
   ```

## ❗ 注意事项

- **IP格式**：必须使用 `IP地址/DUT索引` 格式，如 `192.168.1.120/1`
- **Python架构**：确保Python位数与DLL一致（推荐64位）
- **日志**：仅调试时开启，正常使用关闭节省资源
- **权限**：确保有交换机管理权限
- **操作间隔**：网络配置API（configNet/syncNet/autoStart）操作之间建议间隔30秒，避免交换机资源冲突
- **重试策略**：如遇"无法初始化SFTP会话"等错误，请等待30秒后重试，可能是连接资源未完全释放

## 🔍 故障排除

| 问题             | 解决方案                               |
| ---------------- | -------------------------------------- |
| 连接失败         | 检查IP地址、网络连通性、SSH服务        |
| DLL加载失败      | 确认Python架构，安装VC++运行时         |
| 命令超时         | 增加超时时间，检查网络延迟             |
| 认证失败         | 验证用户名密码，检查账户权限           |
| 配置API操作失败  | 等待30秒后重试，确保相关文件路径有权限 |
| SFTP初始化失败   | 等待30-60秒后重试，可能是SSH资源未释放 |
| 网络配置持续失败 | 尝试重启交换机，或联系技术支持         |

## 📞 技术支持

- **邮箱**: wei.lei@figkey.com
- **官网**: https://www.figkey.com

## 📄 版权声明

版权所有 © 2024 丰柯科技。保留所有权利。

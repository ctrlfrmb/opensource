 # 数据采集 API 说明文档

> 版本：v2.0.0 | 作者：leiwei | 日期：2025-08-27

## 📖 简介

数据采集 API 提供用于 CAN/CANFD 设备数据采集的高性能接口，支持多设备、多通道并行采集，并提供数据缓存、文件存储等功能。

**主要特点**：

- 支持多达 16 个设备的并行采集
- 支持标准 CAN 和 CANFD 协议
- 高精度时间戳记录
- 多通道数据同步采集
- 自动文件分割和管理
- 可配置的采样频率
- 终端电阻控制
- 日志记录和错误诊断

## 📁 目录结构

```bash
bscollector_api/
├── README.pdf                    # 用户说明书（本文档）
├── BSCollectorReleaseNote.txt    # 版本变更记录
├── BSCollectorApi.py             # Python API 封装
├── BSCollectorTest.py            # Python 测试工具
├── BSConfig.ini                  # 配置文件示例
├── BSConfig.py                   # 配置管理类
├── include/
│   ├── bscollectorapi.h          # C/C++ 头文件
│   └── bscollectordef.h
└── lib/
    ├── Winx64/                   # 64位库文件
    │   ├── bscollector_api.dll
    │   ├── bscollector_api.lib
    │   └── ...依赖DLL
    └── Winx86/                   # 32位库文件
        ├── bscollector_api.dll
        ├── bscollector_api.lib
        └── ...依赖DLL
```

注意：运行时需要拷贝依赖库，如common_api、Qt库

## 🚀 快速开始

### 使用测试工具

```bash
# 标准运行
python BSCollectorTest.py --config BSConfig.ini

# 64位python环境下运行：
py -3-64 BSCollectorTest.py --config BSConfig.ini

# 32位python环境下运行：
py -3-32 BSCollectorTest.py --config BSConfig.ini

# 或指定参数
python BSCollectorTest.py --device 1 --channel 1 --baudrate 500000 --serial 0x001D0036 --save
```

### 配置文件示例

```ini
[Device]
device = 1          # 设备索引
channel = 1         # 通道号
baudrate = 500000   # CAN波特率
fdbaudrate = 2000000 # CANFD波特率

[Module1]
serial = 0x001D0036 # 模块序列号
freq = 100          # 采样频率(ms)

[Collection]
arraysize = 500     # 读取数据大小
save = true         # 保存文件
filesize = 5        # 文件大小(MB)
filecount = 10      # 文件数量

[Log]
enable = true       # 启用日志
level = 0           # 日志级别(0:DEBUG)
path = logs/bscollector.log
```

## 🧰 Python开发示例

```python
from BSCollectorApi import BSCollectorApi

# 创建API实例
api = BSCollectorApi()

# 打开设备
api.open_can(1, 1, 500000)  # 设备1，通道1，波特率500kbps

# 设置采样频率
api.set_module_sampling_frequency(0x001D0036, 100)  # 模块序列号，100ms采样频率

# 读取数据
count, data_list = api.read_buffer(500)  # 最多读取500条数据

# 关闭设备
api.close_can()
```

## 🔧 C/C++开发示例

```cpp
#include "bscollectorapi.h"
#include "bscollectordef.h"
#pragma comment(lib, "bscollector_api.lib")

int main()
{
    // Optional: Open log
    ComOpenLog("logs/bscollector.log", 0, 5, 10);  // Debug level, 5MB, 10 files
    
    // Open device (device type 0, index 1, reserved 0)
    int result = BSOpenDev(0, 1, 0);
    if (result != 0) {
        printf("Failed to open device, error code: %d\n", result);
        return -1;
    }
    
    // Initialize CAN channel (device 1, channel 1, 500kbps)
    result = BSInitCAN(1, 1, 500000);
    if (result != 0) {
        printf("Failed to initialize CAN channel, error code: %d\n", result);
        BSCloseDev(1);  // Close device before exit
        return -1;
    }
    
    // Set module sampling frequency (device 1, channel 1, module serial, 100ms)
    result = BSSetModuleSamplingFrequency(1, 1, 0x001D0036, 100);
    if (result != 0) {
        printf("Failed to set sampling frequency, error code: %d\n", result);
        // Continue despite error
    }
    
    // Clear buffer
    BSClearBuffer(1, 1);
    
    // Start saving to file (optional)
    BSStartSaveFile(1, 1, "data.csv", 5, 10);  // 5MB file size, 10 files max
    
    // Read data
    BSCollectData data[500];  // Buffer for 500 data items
    int count = BSReadBuffer(1, 1, data, 500);
    
    // Process data
    if (count > 0) {
        for (int i = 0; i < count; i++) {
            printf("CAN ID: 0x%X, Time: %s\n", data[i].CanID, data[i].Time);
            printf("Data: %d %d %d %d %d %d %d %d\n", 
                  data[i].Data[0], data[i].Data[1], data[i].Data[2], data[i].Data[3],
                  data[i].Data[4], data[i].Data[5], data[i].Data[6], data[i].Data[7]);
        }
    }
    
    // Stop saving file
    BSStopSaveFile(1, 1);
    
    // Close device
    BSCloseDev(1);
    
    // Close log
    ComCloseLog();
    
    return 0;
}
```

## 🔍 数据格式

Python中数据以字典形式返回：

```python
{
    'CanID': 0x123,                         # CAN ID
    'Time': '2025-06-13 15:23:45.678',      # 时间戳
    'Data': [123, 456, 789, 0, 0, 0, 0, 0]  # 8通道数据
}
```

C/C++中使用结构体：

```cpp
typedef struct {
    UINT32_T CanID;       // CAN ID (11位标准或29位扩展)
    char Time[32];        // 时间字符串 "YYYY-MM-DD HH:MM:SS.mmm"
    UINT32_T Rev;         // 保留字段
    int Data[8];          // 采集数据，8个通道
} BSCollectData;
```

## 🧰 API 函数概览

### 日志管理

```cpp
int ComOpenLog(const char *logFile, int level, int maxSize, int maxFiles);
int ComCloseLog();
```

### 设备管理

```cpp
int BSOpenDev(int deviceType, int deviceIndex, int reserved);
int BSCloseDev(int deviceIndex);
```

### CAN 通道管理

```cpp
int BSInitCAN(int deviceIndex, int channelIndex, unsigned int baudRate);
int BSInitCANFD(int deviceIndex, int channelIndex, unsigned int baudRate, unsigned int fdBaudRate);
int BSResetCAN(int deviceIndex, int channelIndex);
int BSSetTerminalResistorCAN(int deviceIndex, int channelIndex, int enable);
```

### 数据采集与存储

```cpp
int BSSetModuleSamplingFrequency(int deviceIndex, int channelIndex, unsigned int serialNumber, int samplingFrequency);
int BSClearBuffer(int deviceIndex, int channelIndex);
int BSStartSaveFile(int deviceIndex, int channelIndex, const char *filePath, int maxSize, int maxFiles);
int BSReadBuffer(int deviceIndex, int channelIndex, BSCollectData* collectDatas, unsigned int dataLen);
int BSStopSaveFile(int deviceIndex, int channelIndex);
```

## ⚠️ 常见问题

| 问题         | 解决方案                                   |
| ------------ | ------------------------------------------ |
| 设备连接失败 | 检查网络连接、设备电源和 IP 地址设置       |
| 读取数据为空 | 确认采样频率设置正确，模块序列号匹配       |
| DLL 加载失败 | 确保依赖库（Qt5Core.dll 等）存在于系统路径 |
| 数据采集延迟 | 调整采样频率，减少其他系统负载             |
| 文件保存失败 | 检查文件路径权限，确保目录可写             |

## ❗ 注意事项

- **设备索引**：设备索引范围为 0x00-0x0F，对应设备 IP 地址 192.168.201.130 - 192.168.201.145
- **采样频率**：采样频率影响数据采集的实时性和资源占用，请根据需要设置合理值
- **缓冲区管理**：长时间采集时应定期调用 `BSReadBuffer` 读取数据，避免缓冲区溢出
- **文件大小**：设置合理的文件大小和数量，避免占用过多磁盘空间
- **日志管理**：生产环境建议关闭日志，以提高性能；调试时可开启详细日志

## 📞 技术支持

- **邮箱**: wei.lei@figkey.com
- **官网**: https://www.figkey.com

## 📄 版权声明

版权所有 © 2025 丰柯科技。保留所有权利。

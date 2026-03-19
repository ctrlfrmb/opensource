# DBC  API 使用说明

> **版本**: v1.0.0 &nbsp;|&nbsp; **日期**: 2025-07-14 &nbsp;|&nbsp; **作者**: leiwei

---

## 1. 简介

DBC 文件解析库，提供纯 C 接口，用于 DBC 文件加载、消息与信号查询、CAN 数据编解码。

数据交换采用 JSON 字符串格式，可被 C/C++、Python、C#、LabVIEW 等语言直接调用。

**主要功能：**

- 多实例并行加载（最大 100 个 DBC 文件）
- 消息与信号查询（按 CAN ID 或名称）
- CAN 数据解码（CAN bytes → 信号物理值）
- CAN 数据编码（信号物理值 → CAN bytes）
- 支持 CAN / CAN FD / BRS / 扩展帧 / 远程帧
- 错误码独占 -15001 ~ -15100 段，避免多 DLL 冲突

---

## 2. 文件结构

    dbc_api/
    ├── DbcClient.py            Python API 封装类
    ├── DbcClientTest.py        Python 交互式测试工具
    ├── DbcMessage.py           Python 错误码和常量定义
    ├── DbcReleaseNote.txt      版本说明
    ├── README.md               本文档
    │
    ├── include/
    │     dbc_api.h             C/C++ API 函数声明（含详细注释）
    │     dbc_api_def.h         错误码、导出宏、类型定义
    │
    └── lib/
          ├── Winx64/           64 位 Windows 库及依赖
          │     dbc_api.dll
          │     dbc_api.lib
          │     common_api.dll
          │     Qt5Core.dll
          │     msvcp140.dll
          │     vcruntime140.dll
          │     ws2_32.dll
          │
          └── Winx86/           32 位 Windows 库及依赖
                （文件同上）

---

## 3. Python 快速测试

确保已安装 Python 3，在 dbc_api 根目录执行：

    python DbcClientTest.py          # 64 位 Python
    py -3-32 DbcClientTest.py        # 32 位 Python

测试命令示例：

    DBC> load vehicle.dbc                                    # 加载 DBC 文件
    DBC> load vehicle.dbc GBK                                # 指定编码加载
    DBC> info                                                # 查看全局信息
    DBC> messages                                            # 消息列表（概要）
    DBC> msg 0x100                                           # 单条消息详情
    DBC> has msg 0x100                                       # 消息是否存在
    DBC> has sig 0x100 EngineSpeed                           # 信号是否存在
    DBC> decode 0x100 12 34 56 78 00 00 00 00                # 解码 CAN 数据
    DBC> decodesig 0x100 EngineSpeed 12 34 56 78 00 00 00 00 # 解码单个信号
    DBC> encodesig 0x100 EngineSpeed 3500.0                  # 编码单个信号
    DBC> unload                                              # 卸载当前实例
    DBC> help                                                # 查看所有命令
    DBC> exit                                                # 退出

---

## 4. C/C++ 集成

### 4.1 项目配置（Visual Studio）

| 配置项     | 设置                                |
| :--------- | :---------------------------------- |
| 包含目录   | 添加 include 目录                   |
| 库目录     | 添加 lib/Winx64 或 lib/Winx86       |
| 链接器输入 | 添加 dbc_api.lib                    |
| 运行部署   | 将 lib 下所有 .dll 复制到 .exe 目录 |

### 4.2 代码示例

    #include "dbc_api.h"
    #include <stdio.h>
    
    int main() {
        /* 1. 可选：开启日志 */
        DBCOpenLog("logs/dbc.log", 1, 10, 5);
    
        /* 2. 加载 DBC 文件 */
        int id = DBCLoadFile("vehicle.dbc", "UTF-8");
        if (id <= 0) {
            printf("加载失败: %d\n", id);
            return -1;
        }
    
        /* 3. 查询 */
        printf("消息数量: %d\n", DBCGetMessageCount(id));
    
        if (DBCHasMessageById(id, 0x100) == 1) {
            printf("消息 0x100 存在\n");
        }
    
        /* 4. 解码 CAN 数据 */
        UINT8_T canData[] = {0x12, 0x34, 0x56, 0x78, 0x00, 0x00, 0x00, 0x00};
        char buf[4096];
        int len = sizeof(buf);
        if (DBCDecode(id, 0x100, canData, 8, buf, &len) == 0) {
            printf("解码结果:\n%s\n", buf);
        }
    
        /* 5. 解码单个信号 */
        UINT64_T rawValue;
        double physValue;
        if (DBCDecodeSignal(id, 0x100, "EngineSpeed",
                            canData, 8, &rawValue, &physValue) == 0) {
            printf("EngineSpeed: raw=%llu, phys=%.2f\n", rawValue, physValue);
        }
    
        /* 6. 编码 (JSON 批量) */
        const char* encJson =
            "{\"signals\":["
            "{\"name\":\"EngineSpeed\",\"physicalValue\":3500.0},"
            "{\"name\":\"Status\",\"rawValue\":1}"
            "]}";
        UINT8_T outData[8];
        int outLen = 8;
        if (DBCEncode(id, 0x100, encJson, outData, &outLen) == 0) {
            printf("编码结果: ");
            for (int i = 0; i < outLen; i++) printf("%02X ", outData[i]);
            printf("\n");
        }
    
        /* 7. 编码单个信号 (物理值，原地修改) */
        UINT8_T data2[8] = {0};
        DBCEncodeSignal(id, 0x100, "EngineSpeed", 3500.0, data2, 8);
    
        /* 8. 卸载 */
        DBCUnloadFile(id);
        DBCCloseLog();
        return 0;
    }

---

## 5. 支持的文件编码

DBCLoadFile 的 fileEncoding 参数支持以下编码格式：

| 编码         | 说明                 |
| :----------- | :------------------- |
| "UTF-8"      | 默认推荐，兼容性最好 |
| "GB2312"     | 简体中文             |
| "GBK"        | 简体中文扩展         |
| "ASCII"      | 纯英文               |
| "UTF-16"     | Unicode 双字节       |
| "UTF-32"     | Unicode 四字节       |
| "ISO-8859-1" | 西欧语言             |
| NULL         | 自动检测编码         |

> 如果 DBC 文件含中文注释或节点名，建议显式指定 "GBK" 或 "GB2312"。

---

## 6. 错误码参考

| 错误码                         |     值 | 说明                  |
| :----------------------------- | -----: | :-------------------- |
| DBC_RESULT_OK                  |      0 | 操作成功              |
| DBC_RESULT_INVALID_PARAM       | -15001 | 无效参数（空指针等）  |
| DBC_RESULT_FILE_NOT_FOUND      | -15002 | DBC 文件未找到        |
| DBC_RESULT_PARSE_FAILED        | -15003 | DBC 文件解析失败      |
| DBC_RESULT_INVALID_INSTANCE_ID | -15004 | 无效的实例 ID         |
| DBC_RESULT_MESSAGE_NOT_FOUND   | -15005 | 消息未找到            |
| DBC_RESULT_SIGNAL_NOT_FOUND    | -15006 | 信号未找到            |
| DBC_RESULT_BUFFER_TOO_SMALL    | -15007 | 缓冲区太小            |
| DBC_RESULT_JSON_PARSE_ERROR    | -15008 | JSON 解析错误         |
| DBC_RESULT_ENCODE_FAILED       | -15009 | 编码失败              |
| DBC_RESULT_INTERNAL_ERROR      | -15099 | 内部错误              |
| DBC_RESULT_TOO_MANY_INSTANCES  | -15100 | 实例数达到上限（100） |

---

## 7. 常见问题

| 问题                   | 解决方案                                                     |
| :--------------------- | :----------------------------------------------------------- |
| Python 加载 DLL 失败   | 确认 Python 位数与 lib 目录匹配（64 位用 Winx64）            |
| 提示缺少 VC++ 运行时   | 安装 Visual C++ Redistributable                              |
| 返回 -15002 文件未找到 | 检查文件路径是否正确，路径使用 UTF-8 编码                    |
| 返回 -15003 解析失败   | DBC 含中文时指定编码，如 "GBK" 或 "GB2312"                   |
| 返回 -15007 缓冲区太小 | 增大 buffer 大小；Python 封装会自动重试                      |
| 编码后其它信号变为 0   | DBCEncode 未列出的信号默认为 0；改用 DBCEncodeSignal 单信号原地编码 |

---

## 8. 技术支持

- **邮箱**：ctrlfrmb@gmail.com

---

*版权所有 (c) 2025. All rights reserved.*

# Agile Modbus

## 1、介绍

Agile Modbus 即：轻量型 modbus 协议栈，满足用户任何场景下的使用需求。

### 1.1、特性

1. 支持 rtu 及 tcp 协议，使用纯 C 开发，不涉及任何硬件接口，可在任何形式的硬件上直接使用。
2. 由于其使用纯 C 开发、不涉及硬件，完全可以在串口上跑 tcp 协议，在网络上跑 rtu 协议。
3. 支持符合 modbus 格式的自定义协议。
4. 同时支持多主机和多从机。
5. 使用简单，只需要将 rtu 或 tcp 句柄初始化好后，调用相应 API 进行组包和解包即可。

### 1.2、目录结构

| 名称 | 说明 |
| ---- | ---- |
| doc | 文档目录 |
| examples | 例子目录 |
| inc  | 头文件目录 |
| src  | 源代码目录 |

### 1.3、许可证

Agile Modbus 遵循 LGPLv2.1 许可，详见 `LICENSE` 文件。

## 2、使用 Agile Modbus

- 帮助文档请查看 [doc/doxygen/Agile_Modbus.chm](./doc/doxygen/Agile_Modbus.chm)

- 用户需要实现硬件接口的 `发送数据` 、 `等待数据接收结束` 、 `清空接收缓存` 函数

  对于 `等待数据接收结束`，提供如下几点思路：

  1. 通用方法

     每隔 20 / 50 ms (该时间可根据波特率和硬件设置，这里只是给了参考值) 从硬件接口读取数据存放到缓冲区中并更新偏移，直到读取不到或缓冲区满，退出读取。

     这对于裸机或操作系统都适用，操作系统可通过 `select` 或 `信号量` 方式完成阻塞。

  2. 串口 `DMA + IDLE` 中断方式

     配置 `DMA + IDLE` 中断，在中断中使能标志，应用程序中判断该标志是否置位即可。

     但该方案容易出问题，数据字节间稍微错开一点时间就不是一帧了。推荐第一种方案。

- 主机：

  1. agile_modbus_rtu_init / agile_modbus_tcp_init 初始化 RTU/TCP 环境
  2. agile_modbus_set_slave 设置从机地址
  3. `清空接收缓存`
  4. `agile_modbus_serialize_xxx` 打包请求数据
  5. `发送数据`
  6. `等待数据接收结束`
  7. `agile_modbus_deserialize_xxx` 解析响应数据
  8. 用户处理得到的数据

- 从机：

  1. 实现 `agile_modbus_slave_callback_t` 类型回调函数
  2. agile_modbus_rtu_init / agile_modbus_tcp_init 初始化 RTU/TCP 环境
  3. agile_modbus_set_slave 设置从机地址
  4. `等待数据接收结束`
  5. agile_modbus_slave_handle 处理请求数据
  6. `清空接收缓存` (可选)
  7. `发送数据`

- `agile_modbus_slave_callback_t` 介绍

  `typedef int (*agile_modbus_slave_callback_t)(agile_modbus_t *ctx, struct agile_modbus_slave_info *slave_info);`

  agile_modbus_slave_info:

  - agile_modbus_sft_t *sft: 包含从机地址和功能码属性，回调中可利用
  - int *rsp_length: 响应数据长度指针，回调中处理 `特殊功能码` 时需要更新其值，否则 **不准更改**
  - int address: 寄存器地址 (不一定用到)
  - int nb: 数目 (不一定用到)
  - uint8_t *buf: 不同功能码需要使用的数据域 (不一定用到)
  - int send_index: 发送缓冲区当前索引 (不一定用到)

### 2.1、示例

使用示例在 [examples](./examples) 下。

### 2.2、Doxygen 文档生成

- 使用 `Doxywizard` 打开 [Doxyfile](./doc/doxygen/Doxyfile) 运行，生成的文件在 [doxygen/output](./doc/doxygen/output) 下。
- 需要更改 `Graphviz` 路径。
- `HTML` 生成未使用 `chm` 格式的，如果使能需要更改 `hhc.exe` 路径。

## 3、联系方式 & 感谢

- 维护：马龙伟
- 主页：<https://github.com/loogg/agile_modbus>
- 邮箱：<2544047213@qq.com>

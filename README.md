# Agile Modbus

## 1、介绍

Agile Modbus 即：轻量型 modbus 协议栈，是我在使用其他第三方库过程中进行归纳而形成的一个库。

### 1.1、特性

1. 其支持 rtu 及 tcp 协议，使用纯 C 开发，不涉及任何硬件接口，可在任何形式的硬件上直接使用。
2. 由于其使用纯 C 开发、不涉及硬件，完全可以在串口上跑 tcp 协议，在网络上跑 rtu 协议。
3. 支持符合 modbus 格式的自定义协议。
4. 同时支持主机和从机
   - 主机时：常用功能码都有相应的 API 支持，支持自定义命令解析。

    ```C
    AGILE_MODBUS_FC_READ_COILS               0x01
    AGILE_MODBUS_FC_READ_DISCRETE_INPUTS     0x02
    AGILE_MODBUS_FC_READ_HOLDING_REGISTERS   0x03
    AGILE_MODBUS_FC_READ_INPUT_REGISTERS     0x04
    AGILE_MODBUS_FC_WRITE_SINGLE_COIL        0x05
    AGILE_MODBUS_FC_WRITE_SINGLE_REGISTER    0x06
    AGILE_MODBUS_FC_READ_EXCEPTION_STATUS    0x07
    AGILE_MODBUS_FC_WRITE_MULTIPLE_COILS     0x0F
    AGILE_MODBUS_FC_WRITE_MULTIPLE_REGISTERS 0x10
    AGILE_MODBUS_FC_REPORT_SLAVE_ID          0x11
    AGILE_MODBUS_FC_MASK_WRITE_REGISTER      0x16
    AGILE_MODBUS_FC_WRITE_AND_READ_REGISTERS 0x17
    ```

    - 从机时：

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

# 使用说明

## 介绍

1. agile_modbus 即：轻量型 modbus 协议栈，是我在使用其他第三方库过程中进行归纳而形成的一个库。
2. 其支持 rtu 及 tcp 协议，使用纯 C 开发，不涉及任何硬件接口，可在任何形式的硬件上直接使用。
3. 由于其使用纯 C 开发、不涉及硬件，完全可以在串口上跑 tcp 协议，在网络上跑 rtu 协议。
4. 支持符合 modbus 格式的自定义协议。
5. 支持常用功能码，如下：

```C
/* Modbus function codes */
#define AGILE_MODBUS_FC_READ_COILS                0x01
#define AGILE_MODBUS_FC_READ_DISCRETE_INPUTS      0x02
#define AGILE_MODBUS_FC_READ_HOLDING_REGISTERS    0x03
#define AGILE_MODBUS_FC_READ_INPUT_REGISTERS      0x04
#define AGILE_MODBUS_FC_WRITE_SINGLE_COIL         0x05
#define AGILE_MODBUS_FC_WRITE_SINGLE_REGISTER     0x06
#define AGILE_MODBUS_FC_READ_EXCEPTION_STATUS     0x07
#define AGILE_MODBUS_FC_WRITE_MULTIPLE_COILS      0x0F
#define AGILE_MODBUS_FC_WRITE_MULTIPLE_REGISTERS  0x10
#define AGILE_MODBUS_FC_REPORT_SLAVE_ID           0x11
#define AGILE_MODBUS_FC_MASK_WRITE_REGISTER       0x16
#define AGILE_MODBUS_FC_WRITE_AND_READ_REGISTERS  0x17
```

6. 使用简单，只需要将 rtu 或 tcp 句柄初始化好后，调用响应 API 进行组包和解包即可。
7. 作为从机使用时只简单提供了解包 API，因为从机的处理多种多样，个人没有能力抽象出一个通用的类。而像 `libmodbus` 里的 `mapping` 结构根本应对不了多少场合。如果您能想到更好的方法，希望能够告知。

## API 介绍

1. 初始化 rtu 句柄

`int agile_modbus_rtu_init(agile_modbus_rtu_t *ctx, uint8_t *send_buf, int send_bufsz, uint8_t *read_buf, int read_bufsz)`

|参数|注释|
|---|---|
|ctx|rtu 句柄|
|send_buf|发送缓冲区地址|
|send_bufsz|发送缓冲区大小|
|read_buf|接收缓冲区地址|
|read_bufsz|接收缓冲区大小|

|返回|注释|
|---|---|
|0|成功|

2. 初始化 tcp 句柄

`int agile_modbus_tcp_init(agile_modbus_tcp_t *ctx, uint8_t *send_buf, int send_bufsz, uint8_t *read_buf, int read_bufsz)`

|参数|注释|
|---|---|
|ctx|tcp 句柄|
|send_buf|发送缓冲区地址|
|send_bufsz|发送缓冲区大小|
|read_buf|接收缓冲区地址|
|read_bufsz|接收缓冲区大小|

|返回|注释|
|---|---|
|0|成功|

3. 设置从机地址

`int agile_modbus_set_slave(agile_modbus_t *ctx, int slave)`

|返回|注释|
|---|---|
|0|成功|

4. 打包请求数据

`agile_modbus_serialize_xxx`

|返回|注释|
|---|---|
|< 0|异常，可能是发送缓冲区太小|
|其他|请求数据长度|

5. 解析响应数据

`agile_modbus_deserialize_xxx`

|返回|注释|
|---|---|
|< 0|异常|
|其他|对应功能码响应对象的长度。如 03 功能码，值代表寄存器个数|

6. 解析主机发来的数据

`int agile_modbus_receive_judge(agile_modbus_t *ctx, int msg_length)`

|参数|注释|
|---|---|
|ctx|句柄|
|msg_length|接收到的数据的长度|

|返回|注释|
|---|---|
|< 0|异常|
|其他|数据格式正确|

## 示例

基于 STM32F103，使用 `RT-Thread Nano`。该 demo 实现了 rtu 及 tcp 的例子，rtu 使用串口作为演示；tcp 使用 ESP8266 演示，支持微信配网。

该 demo 也可作为 `RT-Thread Nano` 学习的参考例程。

1. 示例地址为： [地址](https://github.com/loogg/agile_modbus_demo)
2. 定期维护

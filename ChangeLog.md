# 更新记录

## Agile Modbus 1.1.0 发布

### 新功能

2021-12-02：马龙伟

* 增加 Doxygen 注释，生成文档

2021-12-28：马龙伟

* 增加 RTU 和 TCP 主机例子
* 增加 RTU 和 TCP 从机例子
* 增加示例文档

2022-01-08：马龙伟

* 增加 RTU 点对点传输文件例子
* 增加 RTU 广播传输文件例子

### 修改

2022-01-06：马龙伟

* 修改从机例子，RTU 和 TCP 使用同一个从机回调
* TCP 从机支持最大 5 个客户端接入
* TCP 从机 10s 内未收到正确报文主动断开

2022-01-08：马龙伟

* 去除接收数据判断中长度限制
* 去除 `agile_modbus_serialize_raw_request` 对于原始数据的长度限制

## Agile Modbus 1.1.1 发布

### 修改

2022-06-22：马龙伟

* README.md 增加在 AT32F437 上基于 RT-Thread 实现的支持 Modbus 固件升级的 Bootloader 链接
* 增加 HPM6750_Boot 链接
* 更改 LICENSE 为 `Apache-2.0`

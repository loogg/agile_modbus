# Example description

## 1. Introduction

This example provides functional demonstration of RTU / TCP master and slave.

Using `gcc` under `WSL` or `Linux`, you can directly `make all` to compile all examples and run the test program on your computer.

Directory Structure:

| Name | Description |
| ---- | ---- |
| common | public source code |
| figures | materials |
| rtu_master | RTU master example |
| tcp_master  | TCP master example |
| slave  | RTU + TCP slave example |
| rtu_p2p  | RTU peer-to-peer transfer file |
| rtu_broadcast  | RTU broadcast transmission file (sticky packet processing example) |

## 2. Use

The tools you need to prepare are as follows:

- Virtual serial port software
- Modbus Poll
- Modbus Slave

Type `make clean` 、 `make all` on the command line.

### 2.1. Host

- RTU (rtu_master)

  - Use virtual serial port software to virtualize a pair of serial ports

    ![VirtualCom](./figures/VirtualCom.jpg)

  - Open `Modbus Slave` and set as shown below

    ![ModbusSlaveSetup](./figures/ModbusSlaveSetup.jpg)

  - `Modbus Slave` connection, set as shown below

    ![ModbusSlaveRTUConnection](./figures/ModbusSlaveRTUConnection.jpg)

  - Enter the `rtu_master` directory, `./RtuMaster /dev/ttySX` to run the `RTU` host example, `ttySX` is the other of a pair of virtual serial ports

    ![RTUMaster](./figures/RTUMaster.jpg)

- TCP (tcp_master)

  - Open `Modbus Slave`, `SetUp` settings are consistent with `RTU`

  - `Modbus Slave` connection, set as shown below

    ![ModbusSlaveTCPConnection](./figures/ModbusSlaveTCPConnection.jpg)

  - Enter the `tcp_master` directory, `./TcpMaster 127.0.0.1 502` and run the `TCP` host example

    ![TCPMaster](./figures/TCPMaster.jpg)

### 2.2. Slave machine

- This example (slave) provides both `RTU` and `TCP` slave function demonstrations, controlling the same memory. `TCP` can connect to a maximum of 5 clients. Each client has a no-data timeout of 10s and will be automatically disconnected after 10s.

- The example supports all function codes (except function code 0x07).

- `bit`, `input_bit`, `register`, `input_register` registers are defined separately for each file.

- Use `agile_modbus_slave_util_callback`.

- Register address field:

  | Register | Address range |
  | --- | --- |
  | Coil register | 0x041A ~ 0x0423 (1050 ~ 1059) |
  | Discrete input register | 0x041A ~ 0x0423 (1050 ~ 1059) |
  | Holding register | 0xFFF6 ~ 0xFFFF (65526 ~ 65535) |
  | Input register | 0xFFF6 ~ 0xFFFF (65526 ~ 65535) |

**Note**: Reading and writing other address registers can be successful, but the values ​​are all 0.

use:

- Use virtual serial port software to virtualize a pair of serial ports

  ![VirtualCom](./figures/VirtualCom.jpg)

- Enter the `slave` directory, `./ModbusSlave /dev/ttyS2 1025` to run the example

  /dev/ttySX: One of the virtual serial ports

  1025: Listening port number. If you do not have `root` permissions, the port number must be greater than `1024`

- Open `Modbus Poll` and set and connect RTU as shown below

  ![ModbusPollSetup](./figures/ModbusPollSetup.jpg)

  ![ModbusPollRTUConnection](./figures/ModbusPollRTUConnection.jpg)

- Open 5 `Modbus Poll`, set the same as RTU, and connect as shown below

  ![ModbusPollTCPConnection](./figures/ModbusPollTCPConnection.jpg)

- Effect demonstration

  ![ModbusSlaveShow](./figures/ModbusSlaveShow.jpg)

- Timeout disconnect demonstration

  Close the poll interface of `Modbus Poll` and look at the console print to see the close message.

  ![ModbusSlaveTimeoutShow](./figures/ModbusSlaveTimeoutShow.jpg)

### 2.3. RTU transmission file

![ModbusProtocol](./figures/ModbusProtocol.jpg)

Use `0x50` as the special function code for transferring files.

File data is transferred in packets, with a maximum size of 1024 bytes per packet.

`Data` field protocol definition:

- Host request

  | command | number of bytes | data |
  | ---- | ---- | ---- |
  | 2 Bytes | 2 Bytes | N Bytes |

  Order:

  | Command | Description | Data |
  | ---- | ---- | ---- |
  | 0x0001 | Start sending | File size (4 Bytes) + file name (string) |
  | 0x0002 | Transmission data | Flag (1 Byte) + file data |

  Logo:

  | Status | Description |
  | ---- | ---- |
  | 0x00 | The last packet of data |
  | 0x01 | Not the last packet of data |

- Slave response

  | Command | Status |
  | ---- | ---- |
  | 2 Bytes | 1 Byte |

  state:

  | Status | Description |
  | ---- | ---- |
  | 0x00 | Failure |
  | 0x01 | Success |

- Use virtual serial port software to virtualize 3 serial ports to form a serial port group

  Here I use the MX virtual serial port

  ![VirtualComGroup](./figures/VirtualComGroup.jpg)

#### 2.3.1. Point-to-point transmission

- Enter the `rtu_p2p` directory and open `Linux Shell`. The demonstration effect is as follows

  **Notice**:

  - The transferred files must be general files. Executable files, directories, etc. are not supported.

  - The file path must be the path in `Linux` environment

  - After the slave receives the data, the file name is modified (slave address_original file name) and written in the current directory.

  ![rtu_p2p](./figures/rtu_p2p.gif)

#### 2.3.2. Broadcast transmission

This example mainly demonstrates the use of `frame_length` in `agile_modbus_slave_handle`.

In `broadcast_master`, use broadcast address 0 and send data packets every 5ms. At the same time, 100 bytes of dirty data are sent after each packet of data.

```c

int send_len = agile_modbus_serialize_raw_request(ctx, raw_req, raw_req_len);
serial_send(_fd, ctx->send_buf, send_len);

//dirty data
serial_send(_fd, _dirty_buf, sizeof(_dirty_buf));

```

Under such a fast data flow, `broadcast_slave` must use the `frame_length` parameter in `agile_modbus_slave_handle` to handle sticky packets.

- Enter the `rtu_broadcast` directory and open `Linux Shell`. The demonstration effect is as follows

  **Notice**:

  - The transferred files must be general files. Executable files, directories, etc. are not supported.

  - The file path must be the path in `Linux` environment

  - After the slave receives the data, the file name is modified (slave address_original file name) and written in the current directory.

  ![rtu_broadcast](./figures/rtu_broadcast.gif)

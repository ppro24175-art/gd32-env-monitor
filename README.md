# GD32 多传感器环境监测

裸机 C 课程项目复做。DHT11 通过单线脉宽传输温湿度，BH1750 通过硬件 I²C 传输光照，主控把采样结果打印到串口并通过 ESP8266 ESP-AT 上报 MQTT。工程同时构建 STM32F103C8T6 基线和 GD32F103C8T6 移植目标。

| 验证项 | 当前状态 |
| --- | --- |
| STM32F103 与 GD32F103 | Cortex-M3 交叉编译通过。脚本检查 Flash、初始栈指针及 Thumb 复位入口。 |
| 传感器与 ESP-AT | 主机模拟覆盖正常采样、DHT11 坏校验、DHT11 超时、BH1750 读取失败和断线重连。 |
| MQTT | 本机 Broker 的订阅端收到由固件逻辑生成的 JSON。ESP8266 串口应答由主机模拟。 |
| 硬件 | 已制定采购、接线和点亮步骤。开发板烧录、板间对照、立创 EDA 两层板与打样验证都在实物阶段。 |

## 运行

需要 Python 3.11–3.13 和 Zig 0.15.2。本项目在 Windows x64 和 Python 3.12 上验证。已有 Zig 可直接使用，也可以安装到项目内。

~~~sh
python -m pip install --target .tools/zig ziglang==0.15.2
python build.py firmware all
python build.py test
~~~

烧录镜像在 build/stm32f103.bin 和 build/gd32f103.bin。带调试信息的同名 ELF 可用于符号调试。两块板的 Flash 起始地址为 0x08000000。

~~~sh
python -m pip install --target .tools/mqtt amqtt==0.11.3 paho-mqtt==2.1.0
python sim/mqtt_demo.py --scenario reconnect
~~~

演示在 127.0.0.1:18883 启动 Broker 并打印订阅结果。遇到端口占用可传入 --port 18884。模拟的 ESP-AT 命令在日志中沿用固件配置的 1883。

~~~text
[subscriber] env-monitor/telemetry {"temperature_c":24.4,"humidity_pct":53.0,"light_lux":226.7,"dht11":"ok","bh1750":"ok"}
~~~

## 跟做顺序

1. 跑正常帧和坏校验，手算 DHT11 校验并解释 JSON 中的 null。
2. 让模拟 ESP8266 在第一次上报时断线，定位失败日志、复位动作和第二轮上报。
3. 对照两个芯片的时钟与外设，生成两份镜像并查看复位入口。
4. 开发板到货后，先用串口看传感器，再接 ESP8266 和局域网 Broker。完成两块板对照后，再定两层 PCB。

逐步操作见 [跟做实验](docs/lab.md)。采购型号、接线和点亮步骤见 [硬件联调](hardware/README.md)。技术资料和公开项目见 [参考来源](docs/sources.md)。

## 代码入口

| 文件 | 读代码时关注的内容 |
| --- | --- |
| [firmware/sensors.c](firmware/sensors.c) | 40 位接收、字节校验、I²C 两字节读数和换算 |
| [firmware/monitor.c](firmware/monitor.c) | 采样失败如何进 JSON，采样如何进入 MQTT |
| [firmware/mqtt.c](firmware/mqtt.c) | AT 应答、发布提示符、超时和下一轮恢复 |
| [firmware/board/device.h](firmware/board/device.h) 与 [board.c](firmware/board/board.c) | 芯片目标、时钟、GPIO、硬件 I²C、USART 和微秒计时 |
| [sim/sim.c](sim/sim.c) | 用模拟波形和 AT 应答驱动相同的应用逻辑 |

原创固件和文档采用 [MIT 许可证](LICENSE)。

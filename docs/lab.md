# 从传感器到 MQTT 的跟做实验

## 1 手算一帧数据

~~~sh
python build.py simulate normal
python build.py simulate checksum
~~~

运行前先预测第二条命令会让哪些字段变成 null。打开 [sim.c](../sim/sim.c) 找到 frame。正常帧是 35 00 18 04 51，按十六进制计算 35 + 00 + 18 + 04 = 51。0x35 对应湿度整数 53，0x18 对应温度整数 24，04 对应 0.4 °C。模拟器把坏帧的末字节改成了 50。

跟进 [sensors.c](../firmware/sensors.c) 的 dht11_read。主控拉低约 20 ms，释放总线后等待响应，再用脉宽区分 0 和 1。这里以 48 μs 为分界，等不到下一条边沿就超时。模拟器刻意让正常读取跨过 16 位微秒计数器的回绕，观察代码如何使用无符号差值。

BH1750 是另一条链路。0x23 是七位地址，0x20 是单次高分辨率测量指令。读回 01 10 合成 272，按默认测量参数计算 272 ÷ 1.2，显示 226.7 lx。运行下面两条命令，观察每个故障只影响对应传感器字段。

~~~sh
python build.py simulate dht-timeout
python build.py simulate light-missing
~~~

## 2 跟着一条消息走

~~~sh
python sim/mqtt_demo.py --scenario reconnect
~~~

看 [mqtt.c](../firmware/mqtt.c) 的命令顺序。AT 检查串口，CWJAP 加入 Wi-Fi，MQTTUSERCFG 设置连接参数，MQTTCONN 连接 Broker。MQTTPUBRAW 指定主题和字节数，等到大于号提示符才发送 JSON，最后检查 +MQTTPUB:OK。

沿着日志找到第一次 publish failed。下一轮先复位 ESP，再初始化并重连。主机模拟了串口侧断线，演示脚本随后把成功发布的 JSON 交给真实的本机 Broker。检查带有 subscriber 标记那一行的主题和字段。如果把故障改成 checksum，订阅还会成功吗？运行 python sim/mqtt_demo.py --scenario checksum 验证。

## 3 做一次移植核对

~~~sh
python build.py firmware stm32f103
python build.py firmware gd32f103
~~~

打开 [device.h](../firmware/board/device.h) 和 [board.c](../firmware/board/board.c)。先核对目标条件、外设基址、时钟使能和引脚复用，再看传感器驱动通过 env_io 如何调用它们。

| 功能 | STM32F103C8T6 | GD32F103C8T6 | 怎么检查 |
| --- | --- | --- | --- |
| 时钟 | RCC HSI | RCU IRC8M | 两个目标使用内部 8 MHz，核对 AHB 和 APB 分频。 |
| 微秒计数 | TIM2 | TIMER1 | 预分频 7，16 位计数器每 1 μs 加一。 |
| 光照 | I2C1 | I2C0 | PB6/PB7，100 kHz。看读两字节时 ACK、POS 与 STOP 的顺序。 |
| ESP8266 | USART1 | USART0 | PA9/PA10，115200 8N1。 |
| 串口日志 | USART2 | USART1 | PA2，115200 8N1。 |
| DHT11 与 ESP 复位 | GPIOA / GPIOB | GPIOA / GPIOB | PA0 和 PB0 使用开漏，释放后由上拉提供高电平。 |

两个芯片的部分外设编号不同，对本项目所用实例，基址和关键寄存器布局相同。编译时用不同的目标宏，复用已经核对的寄存器读写。传感器解析与 MQTT 代码通过 env_io 访问目标板。到货后依次替换板卡，用相同的外设、主题和测试顺序记录结果。

构建调试案例。ELF 符号和交叉编译通过以后，烧录镜像还要检查。对比 BIN 开头的两个 32 位小端值和 ELF 入口，第一项应是 0x20005000，第二项应是在 Flash 范围内的奇地址。曾出现镜像头部多出零填充的情况，沿 ELF 程序头找到 Flash 加载地址，再验证导出的复位向量。

## 4 到货后填实测记录

| 场景 | STM32F103 | GD32F103 | 保留证据 |
| --- | --- | --- | --- |
| 复位及串口 | 待测 | 待测 | 芯片丝印、调试器型号、启动日志 |
| DHT11 | 待测 | 待测 | 一次正常值、断开后的日志、可选脉宽截图 |
| BH1750 | 待测 | 待测 | 遮光前后的值、地址 ACK、可选 I²C 波形 |
| MQTT | 待测 | 待测 | AT 版本、订阅消息、断开再恢复的日志 |
| PCB | 待测 | 待测 | ERC/DRC、上电电压和整板联调记录 |

复盘时按信号链讲。先说哪个症状出现、在哪一级找到证据、修改了什么，再说怎么复测以及本次验证覆盖到哪里。

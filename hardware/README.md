# 开发板采购接线与点亮

## 采购清单

| 数量 | 器件 | 核对项 |
| --- | --- | --- |
| 1 | STM32F103C8T6 最小系统开发板 | 核对芯片丝印。引出 SWD、PA0、PA2、PA9、PA10、PB0、PB6、PB7。 |
| 1 | GD32F103C8T6 最小系统开发板 | 核对芯片丝印和相同引脚，确认板载或外接 GD-Link 的连接方式。 |
| 1 | DHT11 三针模块 | 3.3 V 供电规格、引脚顺序和 DATA 上拉。裸件另外准备 4.7 kΩ 电阻。 |
| 1 | BH1750FVI 模块，例如 GY-302 | 核对 3.3 V 供电支持、ADDR 位置、I²C 上拉及上拉电源。 |
| 1 | ESP-WROOM-02D 2 MB 转接板 | 核对 ESP8266 芯片、2 MB Flash，以及 EN、RST、GPIO13、GPIO15 引出。ESP-AT WROOM-02 默认用 GPIO13 和 GPIO15 收发命令。 |
| 各 1 | ST-LINK、GD-Link、3.3 V USB 转 TTL 串口 | 原厂板载调试器可以代替同类外接探针。USB 串口用于日志和 ESP 模块 AT 检查。 |
| 1 组 | 杜邦线、面包板、3.3 V 稳压电源 | ESP8266 电源按厂商文档保证至少 500 mA 可用输出能力，并与 MCU 共地。 |

ESP-WROOM-02D 为本次课程复做的参考模块。先核对转接板的供电、启动拉阻和 UART 引脚。ESP8266 的 AT 下载 UART 与默认命令使用的引脚不同。模块出厂固件可能不同，先发 AT、AT+GMR 和 AT+CMD，检查 MQTTUSERCFG、MQTTCONN 和 MQTTPUBRAW 命令是否可用。

## 接线

两块 MCU 开发板轮流接同一组外设。所有信号按 3.3 V 逻辑接线。

| MCU 引脚 | 对端 | 备注 |
| --- | --- | --- |
| PA0 | DHT11 DATA | DATA 上拉到 3.3 V。 |
| PB6 / PB7 | BH1750 SCL / SDA | SCL、SDA 上拉到 3.3 V。ADDR 置低为 0x23，置高为 0x5C。 |
| PA9 / PA10 | ESP AT RX / TX | 交叉连接。WROOM-02 默认命令端口为 GPIO13 / GPIO15。 |
| PB0 | ESP RST | 开漏复位。核对模块 RST 上拉和 EN 的正常启动状态。 |
| PA2 | USB TTL RX | 串口监看为 115200、8N1。 |
| PA13 / PA14 | 调试器 SWDIO / SWCLK | 连接 GND、目标电压参考和可用的 NRST。 |
| GND | 传感器、ESP8266、串口和调试器 GND | 所有接口共地。 |

## 上电步骤

1. 先连 MCU、DHT11、BH1750 和日志串口。用 STM32CubeProgrammer 与 ST-LINK 烧录 STM32，或用兆易创新发布的适用工具与 GD-Link 烧录 GD32。选择对应 BIN、写入地址 0x08000000，执行校验并复位。用 BOOT0=0 从用户 Flash 启动。
2. 默认配置下固件启用传感器模式。打开串口，核对芯片名与 JSON。遮住 BH1750 再移开，拔掉 DHT11 数据线并检查 timeout。恢复接线后观察下一轮采样。
3. 单独检查 ESP8266 的 AT 端口与固件版本。接回 MCU。复制 firmware/config.example.h 为 firmware/local_config.h，填 2.4 GHz Wi-Fi、电脑局域网 IP、Broker 端口和主题，再编译并烧录。文件会被 Git 忽略，含实际口令的镜像留在本地。
4. 在同一局域网启用仅供实验使用的 MQTT Broker，允许开发板连接其 TCP 端口，并在另一终端订阅主题。例如运行 mosquitto_sub -h <电脑局域网IP> -t env-monitor/telemetry -v。端口 1883 使用明文，实验服务仅放在可信局域网。主机演示脚本用的是本机回环地址，需要从板子联调时按 Broker 文档配置局域网监听。
5. 记录两块板的串口和 MQTT 消息。再临时断开 Wi-Fi，观察下一轮复位、重连及订阅端收到的新消息。固件每轮末尾等待 5 秒，连接失败时 AT 超时会拉长轮询间隔。

## 两层 PCB 验收

用实物确认接口和模块封装后，在立创 EDA 建立 GD32 主控、SWD、BOOT0/NRST、DHT11、BH1750、ESP 与电源原理图。确定稳压器、电容、上下拉和启动配置后跑 ERC。

布局时将 MCU 去耦放近电源脚，给 ESP8266 供电留足线宽和回流路径，给天线留净空。传感器插座布在板边并标注方向。布线后跑 DRC、核验封装和 1 脚、生成 Gerber 与钻孔文件。打样收到后先测电源和短路，再接 SWD、串口及外设，沿用跟做实验的验收表。

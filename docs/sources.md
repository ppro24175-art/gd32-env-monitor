# 参考资料与开源来源

| 原始标题 | 访问日期 | 对应用途 |
| --- | --- | --- |
| [GitHub · Leon19960120/STM32-EnvironmentalMonitor](https://github.com/Leon19960120/STM32-EnvironmentalMonitor) | 2026-11-23 | STM32、DHT11、ESP8266、MQTT 的公开案例。参考功能划分。 |
| [GitHub · kennySzhucheng/EnvMonitor](https://github.com/kennySzhucheng/EnvMonitor) | 2026-11-23 | BH1750 与 PCB 工程组织的公开案例。参考资料组织。 |
| [RM0008 · STM32F101xx, STM32F102xx, STM32F103xx, STM32F105xx and STM32F107xx advanced Arm-based 32-bit MCUs](https://www.st.com/resource/en/reference_manual/rm0008-stm32f103xx-advanced-armbased-32bit-mcus-stmicroelectronics.pdf) | 2026-11-24 | 时钟、GPIO、USART、定时器和 I²C 寄存器。 |
| [STM32F10xxx I²C optimized examples · AN2824](https://www.st.com/resource/en/application_note/an2824-stm32f10xxx-ic-optimized-examples-stmicroelectronics.pdf) | 2026-11-24 | 主接收方读取两个字节的 ACK/POS/STOP 顺序。 |
| [GigaDevice GD32 MCU · GD32F10x 下载](https://www.gd32mcu.com/en/download?kw=GD32F10X) 与 [GD32F10x 用户手册 Rev2.7](https://www.gd32mcu.com/data/documents/userManual/GD32F10x_User_Manual_Rev2.7_CN.pdf) | 2026-11-24 | GD32F10x 文档入口和中容量芯片外设图。 |
| [GD32F103xx Datasheet Rev3.2](https://www.gd32mcu.com/data/documents/datasheet/GD32F103xx%20Datasheet_Rev3.2.pdf) | 2026-11-24 | C8 容量、引脚和器件外设。 |
| [AOSONG 温湿度模块 DHT11 产品手册 v1.3](https://osepp.com/downloads/pdf/DHT11_ASAIRv1.3-2017331.pdf) | 2026-11-24 | 奥松原版资料的公开镜像。单线时序、校验、负温和小数编码。 |
| [ROHM BH1750FVI Technical Note](https://www.mouser.com/datasheet/2/348/bh1750fvi-e-186247.pdf) | 2026-11-24 | ROHM 原版资料的分销商公开镜像。地址、测量指令、等待时间、光照换算。 |
| [ESP8266 ESP-AT MQTT AT Commands](https://docs.espressif.com/projects/esp-at/en/release-v2.2.0.0_esp8266/AT_Command_Set/MQTT_AT_Commands.html) | 2026-11-24 | MQTT 建链、MQTTPUBRAW 字节数和应答。 |
| [ESP8266 ESP-AT Hardware Connection](https://docs.espressif.com/projects/esp-at/en/release-v2.2.0.0_esp8266/Get_Started/Hardware_connection.html) 与 [Released Firmware](https://docs.espressif.com/projects/esp-at/en/release-v2.2.0.0_esp8266/AT_Binary_Lists/ESP8266_AT_binaries.html) | 2026-11-24 | 默认 AT 端口、下载端口和可用固件。 |
| [ESP8266 Hardware Design Guidelines](https://documentation.espressif.com/esp8266_hardware_design_guidelines_en.html) 与 [ESP-WROOM-02D/02U Datasheet](https://documentation.espressif.com/esp-wroom-02u_esp-wroom-02d_datasheet_en.html) | 2026-11-24 | 模块供电、启动、封装、天线与布局。 |

代码与文档为本仓库独立编写。上述 GitHub 项目用于参考，仓库内的实现与构建无需下载示例项目。

同日尝试打开 GD32F10x User Manual Rev2.9 的两个直链。download.gigadevice.com 链接超时，www.gd32mcu.com 链接超过网页工具的单文档大小限制。实现核对使用上表可读取的资料。Zig 官网 robots.txt 无法由网页工具获取，工具链使用公开 PyPI 包安装。

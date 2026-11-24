#ifndef ENV_CONFIG_H
#define ENV_CONFIG_H

/* Copy to local_config.h. An empty SSID enables the sensor-only bring-up. */
#define ENV_WIFI_SSID ""
#define ENV_WIFI_PASSWORD ""
#define ENV_BROKER "192.168.1.20"
#define ENV_PORT 1883
#define ENV_TOPIC "env-monitor/telemetry"
#define ENV_BH1750_ADDRESS 0x23

#endif

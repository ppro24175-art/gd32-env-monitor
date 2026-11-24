#ifndef ENV_MONITOR_H
#define ENV_MONITOR_H

#include "mqtt.h"

void monitor_cycle(const struct env_io *io, struct mqtt_link *link,
                   const struct mqtt_config *mqtt, uint8_t light_address);

#endif

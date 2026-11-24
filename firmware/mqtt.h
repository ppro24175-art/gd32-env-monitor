#ifndef ENV_MQTT_H
#define ENV_MQTT_H

#include "io.h"

struct mqtt_config {
    const char *ssid;
    const char *wifi_password;
    const char *broker;
    const char *client_id;
    const char *topic;
    uint16_t port;
};

struct mqtt_link {
    bool reset_pending;
    bool connected;
};

bool mqtt_connect(struct mqtt_link *link, const struct env_io *io,
                  const struct mqtt_config *config);
bool mqtt_publish(struct mqtt_link *link, const struct env_io *io,
                  const char *topic, const char *payload, size_t length);

#endif

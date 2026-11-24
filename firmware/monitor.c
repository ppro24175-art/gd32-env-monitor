#include "monitor.h"
#include "sensors.h"
#include "text.h"

static void tenths(struct text *out, int32_t value)
{
    if (value < 0) {
        text_byte(out, '-');
        value = -value;
    }
    text_unsigned(out, (uint32_t)value / 10u);
    text_byte(out, '.');
    text_byte(out, (char)('0' + value % 10));
}

static void sensor_field(struct text *out, enum sensor_status status, int32_t value)
{
    if (status == SENSOR_OK) {
        tenths(out, value);
    } else {
        text_add(out, "null");
    }
}

void monitor_cycle(const struct env_io *io, struct mqtt_link *link,
                   const struct mqtt_config *mqtt, uint8_t light_address)
{
    struct humidity_temperature climate = {0};
    uint32_t light = 0;
    const enum sensor_status dht = dht11_read(io, &climate);
    const enum sensor_status bh = bh1750_read(io, light_address, &light);

    char payload[160];
    struct text out = {payload, sizeof(payload), 0, false};
    text_add(&out, "{\"temperature_c\":");
    sensor_field(&out, dht, climate.temperature_tenths);
    text_add(&out, ",\"humidity_pct\":");
    sensor_field(&out, dht, climate.humidity_tenths);
    text_add(&out, ",\"light_lux\":");
    sensor_field(&out, bh, (int32_t)light);
    text_add(&out, ",\"dht11\":\"");
    text_add(&out, sensor_status_name(dht));
    text_add(&out, "\",\"bh1750\":\"");
    text_add(&out, sensor_status_name(bh));
    text_add(&out, "\"}");
    if (out.overflow) {
        io->log("[app] telemetry overflow");
        return;
    }
    io->log(payload);
    if (*mqtt->ssid && mqtt_connect(link, io, mqtt)) {
        (void)mqtt_publish(link, io, mqtt->topic, payload, out.length);
    }
}

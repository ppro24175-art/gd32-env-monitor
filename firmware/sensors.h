#ifndef ENV_SENSORS_H
#define ENV_SENSORS_H

#include "io.h"

enum sensor_status { SENSOR_OK, SENSOR_TIMEOUT, SENSOR_CHECKSUM, SENSOR_FORMAT, SENSOR_I2C };

struct humidity_temperature {
    uint16_t humidity_tenths;
    int16_t temperature_tenths;
};

enum sensor_status dht11_read(const struct env_io *io, struct humidity_temperature *sample);
enum sensor_status dht11_decode(const uint8_t frame[5], struct humidity_temperature *sample);
enum sensor_status bh1750_read(const struct env_io *io, uint8_t address, uint32_t *lux_tenths);
const char *sensor_status_name(enum sensor_status status);

#endif

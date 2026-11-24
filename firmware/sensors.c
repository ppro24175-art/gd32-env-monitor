#include "sensors.h"

static bool wait_for_level(const struct env_io *io, bool level, uint16_t limit_us)
{
    const uint16_t start = io->micros();
    while (io->dht_level() != level) {
        if ((uint16_t)(io->micros() - start) >= limit_us) {
            return false;
        }
    }
    return true;
}

enum sensor_status dht11_decode(const uint8_t frame[5], struct humidity_temperature *sample)
{
    const uint8_t checksum = (uint8_t)(frame[0] + frame[1] + frame[2] + frame[3]);
    if (checksum != frame[4]) {
        return SENSOR_CHECKSUM;
    }
    const uint8_t temperature_fraction = frame[3] & 0x7fu;
    const uint16_t humidity = (uint16_t)(frame[0] * 10u + frame[1]);
    if (frame[1] > 9u || temperature_fraction > 9u || humidity > 1000u || frame[2] > 80u) {
        return SENSOR_FORMAT;
    }
    const int16_t temperature = (int16_t)(frame[2] * 10u + temperature_fraction);
    sample->humidity_tenths = humidity;
    sample->temperature_tenths = (frame[3] & 0x80u) ? -temperature : temperature;
    return SENSOR_OK;
}

enum sensor_status dht11_read(const struct env_io *io, struct humidity_temperature *sample)
{
    uint8_t frame[5] = {0};
    io->dht_drive_low(true);
    io->delay_ms(20);
    io->dht_drive_low(false);

    /* Idle high, sensor response low/high, then 40 MSB-first data bits. */
    if (!wait_for_level(io, true, 10) || !wait_for_level(io, false, 100) ||
        !wait_for_level(io, true, 120) || !wait_for_level(io, false, 120)) {
        return SENSOR_TIMEOUT;
    }
    for (unsigned bit = 0; bit < 40; ++bit) {
        if (!wait_for_level(io, true, 100)) {
            return SENSOR_TIMEOUT;
        }
        const uint16_t rising_edge = io->micros();
        if (!wait_for_level(io, false, 120)) {
            return SENSOR_TIMEOUT;
        }
        const uint16_t high_width = (uint16_t)(io->micros() - rising_edge);
        const unsigned byte = bit / 8u;
        frame[byte] = (uint8_t)((frame[byte] << 1) | (high_width >= 48u));
    }
    return dht11_decode(frame, sample);
}

enum sensor_status bh1750_read(const struct env_io *io, uint8_t address, uint32_t *lux_tenths)
{
    uint8_t bytes[2];
    if (!io->i2c_write_byte(address, 0x20)) { /* One-time high resolution mode */
        return SENSOR_I2C;
    }
    io->delay_ms(200);
    if (!io->i2c_read_pair(address, bytes)) {
        return SENSOR_I2C;
    }
    const uint32_t raw = (uint32_t)bytes[0] * 256u + bytes[1];
    *lux_tenths = (raw * 100u + 6u) / 12u; /* lux = raw / 1.2 */
    return SENSOR_OK;
}

const char *sensor_status_name(enum sensor_status status)
{
    switch (status) {
    case SENSOR_OK: return "ok";
    case SENSOR_TIMEOUT: return "timeout";
    case SENSOR_CHECKSUM: return "checksum";
    case SENSOR_FORMAT: return "format";
    case SENSOR_I2C: return "i2c";
    }
    return "unknown";
}

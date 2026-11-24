#ifndef ENV_IO_H
#define ENV_IO_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* The drivers use this interface on both MCUs and in the host simulator. */
struct env_io {
    uint16_t (*micros)(void);
    uint32_t (*millis)(void);
    void (*delay_ms)(uint32_t ms);
    void (*dht_drive_low)(bool low);
    bool (*dht_level)(void);
    bool (*i2c_write_byte)(uint8_t address, uint8_t value);
    bool (*i2c_read_pair)(uint8_t address, uint8_t data[2]);
    void (*esp_reset)(void);
    bool (*esp_write)(const char *data, size_t length);
    int (*esp_read)(void); /* 0–255, or -1 if no byte is ready. */
    void (*log)(const char *line);
};

#endif

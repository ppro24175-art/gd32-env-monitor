#include "monitor.h"
#include "sensors.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Configure a waveform and an AT transcript, then call the real monitor code. */
static enum { NORMAL, CHECKSUM, DHT_TIMEOUT, LIGHT_MISSING, RECONNECT } scenario;
static uint32_t clock_us;
static uint32_t released_at;
static uint32_t light_command_at;
static bool dht_low;
static bool light_started;
static unsigned reset_count;
static unsigned publish_count;
static size_t expected_payload_length;
static const char *response = "";
static char last_payload[160];
static uint8_t frame[] = {53, 0, 24, 4, 81};

static uint16_t micros(void)
{
    return (uint16_t)clock_us++;
}

static uint32_t millis(void)
{
    return clock_us / 1000u;
}

static void delay_ms(uint32_t ms)
{
    clock_us += ms * 1000u;
}

static void drive_dht(bool low)
{
    if (dht_low && !low) {
        released_at = clock_us;
    }
    dht_low = low;
}

static bool dht_level(void)
{
    if (dht_low) {
        return false;
    }
    if (scenario == DHT_TIMEOUT) {
        return true;
    }
    uint32_t t = clock_us - released_at;
    if (t < 25) {
        return true;
    }
    t -= 25;
    if (t < 80) {
        return false;
    }
    t -= 80;
    if (t < 85) {
        return true;
    }
    t -= 85;
    for (unsigned index = 0; index < 40; ++index) {
        if (t < 54) {
            return false;
        }
        t -= 54;
        const unsigned one = (frame[index / 8u] >> (7u - index % 8u)) & 1u;
        const unsigned high = one ? 72u : 25u;
        if (t < high) {
            return true;
        }
        t -= high;
    }
    return t >= 54;
}

static bool i2c_write(uint8_t address, uint8_t command)
{
    assert(address == 0x23 && command == 0x20);
    light_command_at = clock_us;
    light_started = true;
    return true;
}

static bool i2c_read(uint8_t address, uint8_t data[2])
{
    assert(address == 0x23 && light_started);
    assert(clock_us - light_command_at >= 180000u);
    light_started = false;
    if (scenario == LIGHT_MISSING) {
        return false;
    }
    data[0] = 1;
    data[1] = 16;
    return true;
}

static void reset_esp(void)
{
    ++reset_count;
    expected_payload_length = 0;
    response = "\r\nready\r\n";
    puts("[sim] ESP reset");
}

static bool esp_write(const char *data, size_t length)
{
    if (expected_payload_length) {
        assert(length == expected_payload_length && length < sizeof(last_payload));
        expected_payload_length = 0;
        memcpy(last_payload, data, length);
        last_payload[length] = '\0';
        ++publish_count;
        if (scenario == RECONNECT && publish_count == 1) {
            response = "\r\nWIFI DISCONNECT\r\n+MQTTDISCONNECTED:0\r\n";
        } else {
            response = "\r\n+MQTTPUB:OK\r\n";
            printf("PUBLISH %s\n", last_payload);
        }
        return true;
    }

    char line[256];
    assert(length >= 2 && length < sizeof(line));
    assert(data[length - 2] == '\r' && data[length - 1] == '\n');
    memcpy(line, data, length - 2);
    line[length - 2] = '\0';
    printf("[at] %s\n", line);
    if (strcmp(line, "AT") == 0 || strcmp(line, "ATE0") == 0 ||
        strcmp(line, "AT+CWMODE=1") == 0 ||
        strcmp(line, "AT+MQTTUSERCFG=0,1,\"host-sim\",\"\",\"\",0,0,\"\"") == 0) {
        response = "\r\nOK\r\n";
    } else if (strcmp(line, "AT+CWJAP=\"mock\\,wifi\",\"demo\\\"pass\"") == 0) {
        response = "WIFI CONNECTED\r\nWIFI GOT IP\r\n\r\nOK\r\n";
    } else if (strcmp(line, "AT+MQTTCONN=0,\"127.0.0.1\",1883,0") == 0) {
        response = "\r\n+MQTTCONNECTED:0,1,\"127.0.0.1\",1883,\"\",0\r\n\r\nOK\r\n";
    } else {
        const char prefix[] = "AT+MQTTPUBRAW=0,\"env-monitor/telemetry\",";
        assert(strncmp(line, prefix, sizeof(prefix) - 1) == 0);
        char *end;
        expected_payload_length = strtoul(line + sizeof(prefix) - 1, &end, 10);
        assert(expected_payload_length > 0 && strcmp(end, ",0,0") == 0);
        response = "\r\nOK\r\n> ";
    }
    return true;
}

static int esp_read(void)
{
    clock_us += 100;
    return *response ? (unsigned char)*response++ : -1;
}

static void sim_log(const char *line)
{
    printf("[fw] %s\n", line);
}

int main(int argc, char **argv)
{
    const char *name = argc > 1 ? argv[1] : "normal";
    if (strcmp(name, "checksum") == 0) {
        scenario = CHECKSUM;
        frame[4] ^= 1u;
    } else if (strcmp(name, "dht-timeout") == 0) {
        scenario = DHT_TIMEOUT;
    } else if (strcmp(name, "light-missing") == 0) {
        scenario = LIGHT_MISSING;
    } else if (strcmp(name, "reconnect") == 0) {
        scenario = RECONNECT;
    } else if (strcmp(name, "normal") != 0) {
        fputs("Scenarios: normal checksum dht-timeout light-missing reconnect\n", stderr);
        return 2;
    }
    printf("[sim] %s, DHT11 bytes: %02X %02X %02X %02X %02X, BH1750 data: 01 10\n",
           name, (unsigned)frame[0], (unsigned)frame[1], (unsigned)frame[2],
           (unsigned)frame[3], (unsigned)frame[4]);

    const struct env_io io = {
        .micros = micros, .millis = millis, .delay_ms = delay_ms,
        .dht_drive_low = drive_dht, .dht_level = dht_level,
        .i2c_write_byte = i2c_write, .i2c_read_pair = i2c_read,
        .esp_reset = reset_esp, .esp_write = esp_write, .esp_read = esp_read, .log = sim_log,
    };
    const struct mqtt_config config = {
        .ssid = "mock,wifi", .wifi_password = "demo\"pass", .broker = "127.0.0.1",
        .client_id = "host-sim", .topic = "env-monitor/telemetry", .port = 1883,
    };
    struct mqtt_link link = {.reset_pending = true};

    /* Place the first DHT response across a 16-bit timer wrap. */
    clock_us = 45500;
    monitor_cycle(&io, &link, &config, 0x23);
    if (scenario == RECONNECT) {
        assert(!link.connected && link.reset_pending && publish_count == 1);
        delay_ms(5000);
        monitor_cycle(&io, &link, &config, 0x23);
        assert(reset_count == 2 && publish_count == 2);
    } else {
        assert(reset_count == 1 && publish_count == 1);
    }
    assert(link.connected && !link.reset_pending);

    if (scenario == CHECKSUM || scenario == DHT_TIMEOUT) {
        assert(strstr(last_payload, "\"temperature_c\":null,\"humidity_pct\":null"));
        assert(strstr(last_payload, scenario == CHECKSUM ? "\"checksum\"" : "\"timeout\""));
    } else {
        assert(strstr(last_payload, "\"temperature_c\":24.4,\"humidity_pct\":53.0"));
    }
    assert(strstr(last_payload, scenario == LIGHT_MISSING
                  ? "\"light_lux\":null,\"dht11\":\"ok\",\"bh1750\":\"i2c\""
                  : "\"light_lux\":226.7"));
    return 0;
}

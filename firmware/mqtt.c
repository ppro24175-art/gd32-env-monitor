#include "mqtt.h"
#include "text.h"

static void quoted(struct text *out, const char *s)
{
    text_byte(out, '"');
    for (; *s; ++s) {
        if ((unsigned char)*s < 32u || *s == 127) {
            out->overflow = true;
            return;
        }
        if (*s == '"' || *s == '\\' || *s == ',') {
            text_byte(out, '\\');
        }
        text_byte(out, *s);
    }
    text_byte(out, '"');
}

static bool wait_reply(const struct env_io *io, const char *expected, uint32_t timeout_ms,
                       bool publishing)
{
    char line[80];
    size_t used = 0;
    bool truncated = false;
    const uint32_t start = io->millis();
    while ((uint32_t)(io->millis() - start) < timeout_ms) {
        const int byte = io->esp_read();
        if (byte < 0 || byte == '\r') {
            continue;
        }
        if (byte == '>' && used == 0 && text_equal(expected, ">")) {
            return true;
        }
        if (byte != '\n') {
            if (used + 1 < sizeof(line)) {
                line[used++] = (char)byte;
            } else {
                truncated = true;
            }
            continue;
        }
        line[used] = '\0';
        if (!truncated) {
            const char *reply = line;
            while (*reply == ' ') {
                ++reply;
            }
            if (text_equal(reply, expected)) {
                return true;
            }
            if (text_equal(reply, "ERROR") || text_equal(reply, "FAIL") ||
                text_equal(reply, "+MQTTPUB:FAIL") || text_starts_with(reply, "busy ") ||
                (publishing && (text_equal(reply, "WIFI DISCONNECT") ||
                                text_starts_with(reply, "+MQTTDISCONNECTED:")))) {
                return false;
            }
        }
        used = 0;
        truncated = false;
    }
    return false;
}

static bool command(const struct env_io *io, struct text *out, const char *reply,
                    uint32_t timeout_ms, bool publishing)
{
    text_add(out, "\r\n");
    return !out->overflow && io->esp_write(out->data, out->length) &&
           wait_reply(io, reply, timeout_ms, publishing);
}

static bool simple_command(const struct env_io *io, const char *line)
{
    char buffer[40];
    struct text out = {buffer, sizeof(buffer), 0, false};
    text_add(&out, line);
    return command(io, &out, "OK", 2000, false);
}

bool mqtt_connect(struct mqtt_link *link, const struct env_io *io,
                  const struct mqtt_config *config)
{
    if (link->connected) {
        return true;
    }
    if (link->reset_pending) {
        io->esp_reset();
    }
    link->reset_pending = true;

    /* Discard stale status lines, with a bound even if the module is noisy. */
    for (unsigned n = 0; n < 256 && io->esp_read() >= 0; ++n) {
    }
    if (!simple_command(io, "AT") || !simple_command(io, "ATE0") ||
        !simple_command(io, "AT+CWMODE=1")) {
        io->log("[esp] AT setup failed");
        return false;
    }
    char buffer[256];
    struct text out = {buffer, sizeof(buffer), 0, false};
    text_add(&out, "AT+CWJAP=");
    quoted(&out, config->ssid);
    text_byte(&out, ',');
    quoted(&out, config->wifi_password);
    if (!command(io, &out, "OK", 30000, false)) {
        io->log("[esp] Wi-Fi join failed");
        return false;
    }

    out = (struct text){buffer, sizeof(buffer), 0, false};
    text_add(&out, "AT+MQTTUSERCFG=0,1,");
    quoted(&out, config->client_id);
    text_add(&out, ",\"\",\"\",0,0,\"\"");
    if (!command(io, &out, "OK", 4000, false)) {
        io->log("[esp] MQTTUSERCFG failed");
        return false;
    }
    out = (struct text){buffer, sizeof(buffer), 0, false};
    text_add(&out, "AT+MQTTCONN=0,");
    quoted(&out, config->broker);
    text_byte(&out, ',');
    text_unsigned(&out, config->port);
    text_add(&out, ",0");
    link->connected = command(io, &out, "OK", 20000, true);
    link->reset_pending = !link->connected;
    io->log(link->connected ? "[esp] MQTT connected" : "[esp] MQTT connect failed");
    return link->connected;
}

bool mqtt_publish(struct mqtt_link *link, const struct env_io *io,
                  const char *topic, const char *payload, size_t length)
{
    if (!link->connected || length == 0 || length > 512u) {
        return false;
    }
    char buffer[256];
    struct text out = {buffer, sizeof(buffer), 0, false};
    text_add(&out, "AT+MQTTPUBRAW=0,");
    quoted(&out, topic);
    text_byte(&out, ',');
    text_unsigned(&out, (uint32_t)length);
    text_add(&out, ",0,0");

    if (!command(io, &out, ">", 10000, true) ||
        !io->esp_write(payload, length) ||
        !wait_reply(io, "+MQTTPUB:OK", 12000, true)) {
        link->connected = false;
        link->reset_pending = true;
        io->log("[esp] MQTT publish failed; retry next cycle");
        return false;
    }
    io->log("[esp] MQTT publish accepted");
    return true;
}

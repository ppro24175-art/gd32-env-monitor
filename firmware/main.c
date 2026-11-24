#include "board/board.h"
#include "monitor.h"

#if __has_include("local_config.h")
#include "local_config.h"
#else
#include "config.example.h"
#endif

int main(void)
{
    const struct mqtt_config config = {
        .ssid = ENV_WIFI_SSID, .wifi_password = ENV_WIFI_PASSWORD,
        .broker = ENV_BROKER, .client_id = board_name, .topic = ENV_TOPIC, .port = ENV_PORT,
    };
    struct mqtt_link link = {.reset_pending = true};
    board_init();
    board_io.log(board_name);
    board_io.log(*config.ssid ? "[app] Wi-Fi enabled" : "[app] sensor-only mode");
    board_io.delay_ms(1200);
    for (;;) {
        monitor_cycle(&board_io, &link, &config, ENV_BH1750_ADDRESS);
        board_io.delay_ms(5000);
    }
}

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "wifi.h"
#include "web_server.h"

static const char *TAG = "ESP_APP";
static httpd_handle_t server = NULL;

void app_main() {
    wifi_init();
    wifi_connect();
    server = web_server_init(80);
}
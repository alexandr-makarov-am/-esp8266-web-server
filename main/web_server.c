#include "web_server.h"

static const char *TAG = "WebServer";

static bool web_server_storage_init() {
    size_t total = 0, used = 0;
    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/www",
      .partition_label = NULL,
      .max_files = 1,
      .format_if_mount_failed = true
    };
    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize storage (%s)", esp_err_to_name(ret));
        return false;
    }
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }
    return true;
}

static esp_err_t main_get_handler(httpd_req_t *req) {
    FILE* f = fopen("/www/index.html", "r");
    if (f == NULL) {
        httpd_resp_send(req, "index.html doesn't created.", -1);
        return ESP_FAIL;
    }
    char buff[32];
    while (fgets(buff, sizeof(buff), f) != NULL) {
        httpd_resp_send_chunk(req, buff, -1);
    }
    httpd_resp_send_chunk(req, NULL, 0);
    fclose(f);
    return ESP_OK;
}

static esp_err_t spa_upload_post_handler(httpd_req_t *req) {
    FILE* f = fopen("/www/index.html", "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return ESP_FAIL;
    }
    int content = req->content_len;
    while (content > 0) {
        char buf[32];
        int ret = httpd_req_recv(req, buf, sizeof(buf));
        if (ret <= 0) {
             if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                continue;
            }
            return ESP_FAIL;
        }
        fwrite(buf, sizeof(char), ret, f);
        content -= ret;
    }
    fclose(f);
    httpd_resp_send(req, "index.html was saved.", -1);
    return ESP_OK;
}

static httpd_uri_t main_get = {
    .uri      = "/",
    .method   = HTTP_GET,
    .handler  = main_get_handler
};

static httpd_uri_t spa_post = {
    .uri      = "/spa/upload",
    .method   = HTTP_POST,
    .handler  = spa_upload_post_handler
};

httpd_handle_t web_server_init(int port) {
    httpd_handle_t server;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = port;
    ESP_LOGI(TAG, "Starting server...");
    if (httpd_start(&server, &config) == ESP_OK) {
        ESP_LOGI(TAG, "Started server on port: '%d'", config.server_port);
        if (web_server_storage_init()) {
            ESP_LOGI(TAG, "Server storage succesfully initialized.");
        }
        httpd_register_uri_handler(server, &main_get);
        httpd_register_uri_handler(server, &spa_post);
        return server;
    }
    ESP_LOGE(TAG, "Error starting server!");
    return NULL;
}
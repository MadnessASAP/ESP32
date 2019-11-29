/*
 *   This file is part of DroneBridge: https://github.com/seeul8er/DroneBridge
 *
 *   Copyright 2018 Wolfgang Christl
 *   Rewritten to use ESP-IDF HTTP Server component
 *   by Michael "ASAP" Weinrich <michael@a5ap.net>
 * 
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 */


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "globals.h"
#include "esp_log.h"
#include "nvs.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include <stdio.h>

#define TAG "SETTINGS_SERVER"

void write_settings_to_nvs() {
    ESP_LOGI(TAG, "Saving to NVS");
    nvs_handle my_handle;
    ESP_ERROR_CHECK(nvs_open("settings", NVS_READWRITE, &my_handle));
    ESP_ERROR_CHECK(nvs_set_str(my_handle, "ssid", (char *) DEFAULT_SSID));
    ESP_ERROR_CHECK(nvs_set_str(my_handle, "wifi_pass", (char *) DEFAULT_PWD));
    ESP_ERROR_CHECK(nvs_set_u8(my_handle, "wifi_chan", DEFAULT_CHANNEL));
    ESP_ERROR_CHECK(nvs_set_u32(my_handle, "baud", DB_UART_BAUD_RATE));
    ESP_ERROR_CHECK(nvs_set_u8(my_handle, "gpio_tx", DB_UART_PIN_TX));
    ESP_ERROR_CHECK(nvs_set_u8(my_handle, "gpio_rx", DB_UART_PIN_RX));
    ESP_ERROR_CHECK(nvs_set_u8(my_handle, "proto", SERIAL_PROTOCOL));
    ESP_ERROR_CHECK(nvs_set_u16(my_handle, "trans_pack_size", TRANSPARENT_BUF_SIZE));
    ESP_ERROR_CHECK(nvs_set_u8(my_handle, "ltm_per_packet", LTM_FRAME_NUM_BUFFER));
    ESP_ERROR_CHECK(nvs_commit(my_handle));
    nvs_close(my_handle);
}

static esp_err_t settings_handler(httpd_req_t *request) {
    // TODO: Actually search and replace placeholder tags with the actual values from current settings
    ESP_LOGD(TAG":settings_handler()", "GET: %s", request->uri);
    
    bool chunk_resp = false;
    FILE * f = fopen("/www/settings.html", "r");
    if(f == NULL) {
        httpd_resp_send_404(request);
        return ESP_OK;
    }

    do {
        char buf[1024];
        size_t bytes_read = fread(buf, 1, 1024, f);
        if(bytes_read == 1024) {
            chunk_resp = true;
            ESP_ERROR_CHECK_WITHOUT_ABORT(httpd_resp_send_chunk(request, buf, bytes_read));
        } else if (bytes_read < 1024 && !ferror(f)) {
            if(chunk_resp) ESP_ERROR_CHECK_WITHOUT_ABORT(httpd_resp_send_chunk(request, buf, bytes_read));
            else ESP_ERROR_CHECK_WITHOUT_ABORT(httpd_resp_send(request, buf, bytes_read));
        } else httpd_resp_send_500(request);
    } while(!(feof(f) || ferror(f)));
    
    fclose(f);
    if(chunk_resp) ESP_ERROR_CHECK_WITHOUT_ABORT(httpd_resp_send_chunk(request, NULL, 0));
    
    return ESP_OK;        
}

httpd_uri_t uri_settings = {
    .uri = "/settings.html",
    .method = HTTP_GET,
    .handler = settings_handler,
    .user_ctx = NULL
};

static httpd_handle_t _start_webserver() {
    httpd_handle_t server = NULL;
    httpd_config_t server_config = HTTPD_DEFAULT_CONFIG();

    ESP_LOGI(TAG, "starting http server");
    esp_err_t retval = httpd_start(&server, &server_config);
    if (retval == ESP_OK)
    {
        ESP_LOGI(TAG, "registering URI handlers");
        httpd_register_uri_handler(server, &uri_settings);
        return server;
    } else {
        ESP_LOGE(TAG, "failed to start http server: %s", esp_err_to_name(retval));
        return NULL;
    }
    
    
}

// Called when WiFi connects and receives an IP address
static void _connect_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    httpd_handle_t * server = (httpd_handle_t*) arg;
    if (*server == NULL) *server = _start_webserver();
    
}

// Called if WiFi is disconnected
static void _disconnect_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    // TODO stop webserver
}

void http_dashboard_server(void *parameter) {
    ESP_LOGI(TAG, "http_dashboard_server task started");
    static httpd_handle_t server = NULL;

    // Register handlers for wifi connect and disconnect events
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &_connect_handler, &server));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, SYSTEM_EVENT_AP_START, &_connect_handler, &server));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &_disconnect_handler, &server));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, SYSTEM_EVENT_AP_STOP, &_disconnect_handler, &server));
    
    // Open spiffs webroot partition

       
    while (1) {
        vTaskSuspend(NULL);
    }
     
    ESP_LOGI(TAG, "...tcp_client task closed\n");
    vTaskDelete(NULL);
}


/**
 * @brief Starts a HTTP server that serves the page to change settings & handles the changes
 */
void start_dashboard_server() {
    xTaskCreate(&http_dashboard_server, "http_dashboard_server", 10240, NULL, 5, NULL);
}

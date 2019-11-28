// DroneBridge display module
//
// Uses an attached display module to provide feedback on drone & bridge status
// Author Michael "ASAP" Weinrich <michael@a5ap.net>

#include <cstdio>
#include <string>
#include <sstream>
#include <iomanip>
#include <set>
#include <tuple>
#include "db_display.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/timer.h"
#include "esp_log.h"
#include "SSD1306IDF.h"

#define TAG "oled"
#define OLED_ADDRESS 0x3c
#define OLED_SDA GPIO_NUM_4
#define OLED_SCL GPIO_NUM_15
#define OLED_RST 16

typedef std::tuple<db_err_level_t, std::string, std::string> _message_t;
std::multiset<_message_t> messege_queue;

static void reset_display() {
    gpio_config_t reset_pin_config = {
        .pin_bit_mask = 1 << OLED_RST,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&reset_pin_config);
    gpio_set_level((gpio_num_t) OLED_RST, 0); // low
    vTaskDelay(20 / portTICK_PERIOD_MS); // wait 20 ms
    gpio_set_level((gpio_num_t) OLED_RST, 1); // high
}

extern "C" void display_status(void *) {
    ESP_LOGI(TAG, "display task started");
    SSD1306IDF display(OLED_ADDRESS, OLED_SDA, OLED_SCL);
    reset_display();
    
    display.init();
    display.setFont(DejaVu_Sans_Mono_8);
    ESP_LOGI(TAG, "display init done");
    constexpr auto refresh_delay = 200 / portTICK_PERIOD_MS; // 5 Updates / Second = 1 update every 200 ms 
    TickType_t lastWake = xTaskGetTickCount();
    while (1) {
        display.clear();
        display.drawString(0, 0, "db");
        
        display.setTextAlignment(TEXT_ALIGN_RIGHT);
        std::stringstream timeString;
        auto uptime_us = esp_timer_get_time();
        int uptime_s = uptime_us / 1e6; //  1 second = 1,000,000 microseconds
        int uptime_min = uptime_s / 60;
        int uptime_hour = uptime_min / 60;
        uptime_s %= 60; uptime_min %= 60;
        timeString << uptime_hour << ":"
            << std::setfill('0') << std::setw(2) << uptime_min << ":"
            << std::setfill('0') << std::setw(2)  << uptime_s;
        display.drawString(127, 0, timeString.str());

        display.setTextAlignment(TEXT_ALIGN_LEFT);
        std::stringstream wifiString;
        wifiString << "wifi t " 
            << std::setw(3) << db_status.wifi.tx_data
            << " r " <<  std::setw(3) << db_status.wifi.rx_data;
        display.drawString(0, 10, wifiString.str());
        db_status.wifi.tx_data = 0; db_status.wifi.rx_data = 0;

        std::stringstream uartString;
        uartString << "uart t " 
            << std::setw(3) << db_status.uart.tx_data
            << " r " <<  std::setw(3) << db_status.uart.rx_data;
        display.drawString(0, 20, uartString.str());
        db_status.uart.tx_data = 0; db_status.uart.rx_data = 0;

        display.display();
        vTaskDelayUntil(&lastWake, refresh_delay);
    }
}

int display_message(char * tag, db_err_level_t level, char * format, ...) {
    
    _message_t msg(level, std::string(tag), std::string());;
    va_list varg;
    size_t msg_size = snprintf(nullptr, 0, format, varg) + 1; // +1 for the \0 terminal
    char *buf = new char[msg_size];
    snprintf(buf, msg_size, format, varg);
    std::get<2>(msg) + std::string(buf);
    return 0;
}

extern "C" void display_service() {
    xTaskCreate(display_status, "display", 8192, NULL, 1, NULL);
}

db_status_t db_status = {
    .wifi = { 0, 0, 0, 0, 0, 0 },
    .uart = { 0, 0, 0, 0, 0, 0 },
    .display_msg = display_message 
};
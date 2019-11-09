// Copyright 2017-2019 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "esp_err.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_wifi_types.h"

#include "board.h"
#include "btpart.h"
#include "wifipart.h"
#include "httpdpart.h"
#include "nvs_config.h"
#include "mqttpart.h"

#define TAG "BLE_MESH_WIFI_COEXIST_DEMO"

extern x_config_t config;

void app_main(void)
{
    esp_err_t err;
    static httpd_handle_t server = NULL;
    /*wifi_mode_t wifi_mode = WIFI_MODE_AP;
    char* wifi_ssid = "";
    char* wifi_pass = "";
    uint ssid_len;
    uint pass_len;
    bool bt_enabled = false;*/

    ESP_LOGI(TAG, "Initializing...");

    err = board_init();
    if (err) {
        ESP_LOGE(TAG, "board_init failed (err %d)", err);
        return;
    }

    gpio_set_level(LED_PIN, 1);

    get_config();

    wifi_init(config.wifi_mode, config.ssid, config.pass);

    if (config.bt_state) {
    	err = bluetooth_init();
    	if (err) {
    		ESP_LOGE(TAG, "esp32_bluetooth_init failed (err %d)", err);
    		return;
    	}
        /* Initialize the Bluetooth Mesh Subsystem */
        err = ble_mesh_init();
        if (err) {
            ESP_LOGE(TAG, "Bluetooth mesh init failed (err %d)", err);
            return;
        }
    }

    server = start_webserver();

    if (config.mqtt_state) {
    	mqtt_init();
    }

    gpio_set_level(LED_PIN, 0);

}


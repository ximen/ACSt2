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

#include <stdio.h>
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "board.h"
#include "nvs_config.h"
#include "esp_ble_mesh_local_data_operation_api.h"
#include "esp_ble_mesh_defs.h"
#define TAG "BOARD"

extern x_config_t config;

struct k_delayed_work prov_timer;

gpio_num_t outputs[OUTPUT_NUM] = {
		GPIO_NUM_12,
		GPIO_NUM_13,
		GPIO_NUM_14,
		GPIO_NUM_15,
		GPIO_NUM_16,
		GPIO_NUM_17,
		GPIO_NUM_18,
		GPIO_NUM_19,
		GPIO_NUM_21,
		GPIO_NUM_22
};

void timer_stop(struct k_work *work){
	ESP_LOGI(TAG, "Erasing provision data");
	bt_mesh_reset();
	ESP_ERROR_CHECK(nvs_flash_erase());
	esp_restart();
}

void led_off(struct k_work *work){
	gpio_set_level(LED_PIN, 0);
}

void board_prov_complete(void){
	gpio_set_level(LED_PIN, 1);
	k_delayed_work_init(&prov_timer, led_off);
	k_delayed_work_submit(&prov_timer, PROVISION_LED_INTERVAL);
}

static void board_led_init(void)
{
    for (int i = 0; i < OUTPUT_NUM; i++) {
        gpio_pad_select_gpio(outputs[i]);
        gpio_set_direction(outputs[i], GPIO_MODE_INPUT_OUTPUT);
        if (ACTIVE_LEVEL == 1) gpio_set_pull_mode(outputs[i], GPIO_PULLUP_ENABLE); else gpio_set_pull_mode(outputs[i], GPIO_PULLDOWN_ENABLE);
        gpio_set_level(outputs[i], OFF_LEVEL);
    }
    gpio_pad_select_gpio(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_pull_mode(LED_PIN, GPIO_PULLDOWN_ENABLE);
    gpio_set_level(LED_PIN, 0);

    gpio_config_t io_conf;
    //interrupt of any edge
    io_conf.intr_type = GPIO_PIN_INTR_ANYEDGE;
    //bit mask of the pins, use button pin here
    io_conf.pin_bit_mask = 1ULL<<BUTTON_PIN;
    //set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);
}

uint8_t get_output_by_pin(gpio_num_t pin){
	for (uint8_t i=0; i<OUTPUT_NUM; i++){
		if(outputs[i] == pin) return i;
	}
	return 0;
}

esp_err_t board_init()
{
	board_led_init();

    return ESP_OK;
}

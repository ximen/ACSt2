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

#ifndef _BOARD_H_
#define _BOARD_H_

#include "sdkconfig.h"
#include "driver/gpio.h"
#include "mesh_kernel.h"
#include "esp_ble_mesh_defs.h"

#define LED_PIN 		GPIO_NUM_16
#define BUTTON_PIN 		GPIO_NUM_0
#define ACTIVE_LEVEL 	0
#define ON_LEVEL		ACTIVE_LEVEL
#define OFF_LEVEL		!ACTIVE_LEVEL
#define OUTPUT_NUM 		2		// Number of output ports on the board
#define CID_ESP     	0x02E5
#define CID_NVAL    	0xFFFF
#define PROVISION_LED_INTERVAL 2000
#define INIT_LED_INTERVAL 250
#define UNPROVISION_BUTTON_INTERVAL 3000
#define ESP_INTR_FLAG_DEFAULT 0

//void board_output_number(esp_ble_mesh_output_action_t action, uint32_t number);

void led_off(struct k_work *work);
void timer_stop(struct k_work *work);
void board_prov_complete(void);
uint8_t get_output_by_pin(gpio_num_t pin);
//void board_led_operation(uint8_t pin, uint8_t onoff);

esp_err_t board_init();

#endif

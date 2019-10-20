/*
 * nvs_config.h
 *
 *  Created on: 21 июл. 2019 г.
 *      Author: ximen
 */

#ifndef MAIN_NVS_CONFIG_H_
#define MAIN_NVS_CONFIG_H_

#include "board.h"

typedef struct{
	uint8_t wifi_mode;
	char ssid[32];
	char pass[32];
	uint8_t bt_state;
	uint8_t mqtt_state;
	char mqtt_broker[32];
	char mqtt_prefix[32];
	char mqtt_topics[OUTPUT_NUM][64];
} x_config_t;

void get_config();
void set_config(x_config_t config);
esp_err_t save_config();
void print_conf();

#endif /* MAIN_NVS_CONFIG_H_ */

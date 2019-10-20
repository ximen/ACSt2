/*
 * nvs_config.c
 *
 *  Created on: 21 июл. 2019 г.
 *      Author: ximen
 */
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "esp_err.h"
#include "esp_wifi_types.h"

#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs_config.h"

#define TAG "NVS_CONFIG"
#define PARTITION "nvs"

nvs_handle_t handle;
x_config_t config;

void print_conf(){
	ESP_LOGI(TAG, "Current config:\n"
			"WiFi mode: %d\n"
			"SSID: %s\n"
			"Pass: %s\n"
			"BT: %d\n"
			"MQTT: %d\n"
			"Broker: %s"
			"Prefix: %s", config.wifi_mode, config.ssid, config.pass, config.bt_state, config.mqtt_state, config.mqtt_broker, config.mqtt_prefix);
	for(uint8_t i = 0; i < OUTPUT_NUM; i++)
		ESP_LOGI(TAG, "Topic %d: %s", i+1, &config.mqtt_topics[i][0]);
}

wifi_mode_t get_wifi_mode(){
	esp_err_t err;
    ESP_LOGI(TAG, "Reading wifi state from NVS ... ");
    uint8_t wifi_mode = 0;
    err = nvs_get_u8(handle, "wifi_mode", &wifi_mode);
    if (err != ESP_OK) ESP_LOGE(TAG, "Error (%s) reading!\n", esp_err_to_name(err));
    if((wifi_mode == 0)||(wifi_mode > 3)) wifi_mode = WIFI_MODE_AP;
    return (wifi_mode_t)wifi_mode;
}

bool get_bt_state(){
	esp_err_t err;
    ESP_LOGI(TAG, "Reading BT state from NVS ... ");
    uint8_t bt_enabled = 0;
    err = nvs_get_u8(handle, "bt_enabled", &bt_enabled);
    if (err != ESP_OK) ESP_LOGE(TAG, "Error (%s) reading!\n", esp_err_to_name(err));
    return (bool)bt_enabled;
}

bool get_mqtt_state(){
	esp_err_t err;
    ESP_LOGI(TAG, "Reading MQTT state from NVS ... ");
    uint8_t mqtt_enabled = 0;
    err = nvs_get_u8(handle, "mqtt_enabled", &mqtt_enabled);
    if (err != ESP_OK) ESP_LOGE(TAG, "Error (%s) reading!\n", esp_err_to_name(err));
    return (bool)mqtt_enabled;
}

void get_wifi_ssid(char *ssid, uint *length){
	esp_err_t err;
    ESP_LOGI(TAG, "Reading BT state from NVS ... ");
    err = nvs_get_str(handle, "wifi_ssid", ssid, length);
    if (err != ESP_OK) ESP_LOGE(TAG, "Error (%s) reading!\n", esp_err_to_name(err));
    return;
}

void get_mqtt_broker(char *broker, uint *length){
	esp_err_t err;
    ESP_LOGI(TAG, "Reading MQTT broker address from NVS ... ");
    err = nvs_get_str(handle, "mqtt_broker", broker, length);
    if (err != ESP_OK) ESP_LOGE(TAG, "Error (%s) reading!\n", esp_err_to_name(err));
    return;
}

void get_mqtt_prefix(char *prefix, uint *length){
	esp_err_t err;
    ESP_LOGI(TAG, "Reading MQTT prefix from NVS ... ");
    err = nvs_get_str(handle, "mqtt_prefix", prefix, length);
    if (err != ESP_OK) ESP_LOGE(TAG, "Error (%s) reading!\n", esp_err_to_name(err));
    return;
}

void get_wifi_pass(char *pass, uint *length){
	esp_err_t err;
    ESP_LOGI(TAG, "Reading BT state from NVS ... ");
    err = nvs_get_str(handle, "wifi_pass", pass, length);
    if (err != ESP_OK) ESP_LOGE(TAG, "Error (%s) reading!\n", esp_err_to_name(err));
    return;
}

void get_config(){
	esp_err_t err;
	config.wifi_mode = 0;
	//config.ssid = "";
	//config.pass = "";
	config.bt_state = 1;
	config.mqtt_state = 0;

	err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    err = nvs_open(PARTITION, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        return;
    } else {
    	config.wifi_mode = get_wifi_mode();
    	if (config.wifi_mode == WIFI_MODE_STA){
    		uint ssid_len = 32;
    		uint pass_len = 32;
    		get_wifi_ssid(config.ssid, &ssid_len);
    		get_wifi_pass(config.pass, &pass_len);
    		if ((ssid_len < 2)||(ssid_len > 32)||(pass_len < 8)) config.wifi_mode = WIFI_MODE_AP;
    	}
    	config.bt_state = get_bt_state();
    	if(config.bt_state && (config.wifi_mode == WIFI_MODE_AP)) config.bt_state = 0;
    	config.mqtt_state = get_mqtt_state();
    	uint broker_len = 32;
    	get_mqtt_broker(config.mqtt_broker, &broker_len);
    	uint prefix_len = 32;
    	get_mqtt_prefix(config.mqtt_prefix, &prefix_len);
    	char topic_buf[13];
    	for(uint8_t i = 0; i < OUTPUT_NUM; i++){
    		uint8_t length = 64;
    		snprintf(topic_buf, 13, "mqtt_topic%d", i+1);
    	    ESP_LOGI(TAG, "Reading topic %d from NVS ... ", i+1);
    	    err = nvs_get_str(handle, topic_buf, config.mqtt_topics[i], &length);
    	    if (err != ESP_OK) ESP_LOGE(TAG, "Error (%s) reading topic %d!\n", esp_err_to_name(err), i+1);
    	}
    	nvs_close(handle);
    }
    print_conf();
    return;
}

esp_err_t save_config(){
	esp_err_t err;
	print_conf();
    err = nvs_open(PARTITION, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        return err;
    } else {
    	err = nvs_set_u8(handle, "wifi_mode", config.wifi_mode);
    	if(err != ESP_OK) ESP_LOGE(TAG, "Error (%s) writing WiFI mode!\n", esp_err_to_name(err));
    	err = nvs_set_str(handle, "wifi_ssid", config.ssid);
    	if(err != ESP_OK) ESP_LOGE(TAG, "Error (%s) writing WiFI SSID!\n", esp_err_to_name(err));
    	err = nvs_set_str(handle, "wifi_pass", config.pass);
    	if(err != ESP_OK) ESP_LOGE(TAG, "Error (%s) writing WiFI password!\n", esp_err_to_name(err));
    	err = nvs_set_u8(handle, "bt_enabled", config.bt_state);
    	if(err != ESP_OK) ESP_LOGE(TAG, "Error (%s) writing BT state!\n", esp_err_to_name(err));
    	err = nvs_set_u8(handle, "mqtt_enabled", config.mqtt_state);
    	if(err != ESP_OK) ESP_LOGE(TAG, "Error (%s) writing MQTT state!\n", esp_err_to_name(err));
    	err = nvs_set_str(handle, "mqtt_broker", config.mqtt_broker);
    	if(err != ESP_OK) ESP_LOGE(TAG, "Error (%s) writing MQTT Broker!\n", esp_err_to_name(err));
    	err = nvs_set_str(handle, "mqtt_prefix", config.mqtt_prefix);
    	if(err != ESP_OK) ESP_LOGE(TAG, "Error (%s) writing MQTT Prefix!\n", esp_err_to_name(err));
    	char topic_buf[13];
    	for(uint8_t i = 0; i < OUTPUT_NUM; i++){
    		snprintf(topic_buf, 13, "mqtt_topic%d", i+1);
    		err = nvs_set_str(handle, topic_buf, config.mqtt_topics[i]);
    		if(err != ESP_OK) ESP_LOGE(TAG, "Error (%s) writing topic %d!\n", esp_err_to_name(err), i+1);
    	}
    }
    nvs_close(handle);
    return err;
}


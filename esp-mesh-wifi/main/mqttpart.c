/*
 * mqttpart.c
 *
 *  Created on: 30 июл. 2019 г.
 *      Author: ximen
 */

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"
#include "mqttpart.h"
#include "nvs_config.h"
#include "btpart.h"

extern x_config_t config;
#define TAG "MQTT_PART"
//#define TOPIC_PREFIX "/home/"
#define TOPIC_CMD "/set"
#define TOPIC_STATE "/state"
#define TOPIC_AVAIL "/available"

extern gpio_num_t outputs[OUTPUT_NUM];
esp_mqtt_client_handle_t client;

static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    // your_context_t *context = event->context;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
//            for(uint8_t i = 0; i < OUTPUT_NUM; i++){
//            	char topic_buf[strlen(TOPIC_PREFIX) + strlen(config.mqtt_topics[i]) + strlen(TOPIC_AVAIL)];
//            	ESP_LOGI(TAG, "Publishing topic %s", topic_buf);
//            	snprintf(topic_buf, sizeof(topic_buf), TOPIC_PREFIX"%s"TOPIC_AVAIL, config.mqtt_topics[i]);
//            	esp_mqtt_client_publish(client, topic_buf, "data_3", 0, 1, 1);
//            }

            for(uint8_t i = 0; i < OUTPUT_NUM; i++){
            	if(strlen(config.mqtt_topics[i]) > 0){
            		char topic_buf[strlen(config.mqtt_prefix) + strlen(config.mqtt_topics[i]) + strlen(TOPIC_STATE)+1];
            		snprintf(topic_buf, sizeof(topic_buf), "%s%s"TOPIC_STATE, config.mqtt_prefix, config.mqtt_topics[i]);
            		ESP_LOGI(TAG, "Publishing topic %s", topic_buf);
            		if (gpio_get_level(outputs[i]) == ON_LEVEL)
            			esp_mqtt_client_publish(client, topic_buf, "ON", 0, 1, 1);
            		else
            			esp_mqtt_client_publish(client, topic_buf, "OFF", 0, 1, 1);
            	}
            }

            for(uint8_t i = 0; i < OUTPUT_NUM; i++){
            	if(strlen(config.mqtt_topics[i]) > 0){
            		char topic_buf[strlen(config.mqtt_prefix) + strlen(config.mqtt_topics[i]) + strlen(TOPIC_CMD)+1];
            		snprintf(topic_buf, sizeof(topic_buf), "%s%s"TOPIC_CMD, config.mqtt_prefix, config.mqtt_topics[i]);
            		ESP_LOGI(TAG, "Subscribing topic %s", topic_buf);
            		esp_mqtt_client_subscribe(client, topic_buf, 0);
            	}
            }
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;

        case MQTT_EVENT_SUBSCRIBED:
//            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
//            msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
//            ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            //ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            //ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            //ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
            char* topic = malloc(event->topic_len+1);
            char* data = malloc(event->data_len+1);
            strlcpy(topic, event->topic, event->topic_len+1);
            strlcpy(data, event->data, event->data_len+1);
            for(uint8_t i = 0; i < OUTPUT_NUM; i++){
        		char topic_buf_set[strlen(config.mqtt_prefix) + strlen(config.mqtt_topics[i]) + strlen(TOPIC_CMD)+1];
        		char topic_buf_state[strlen(config.mqtt_prefix) + strlen(config.mqtt_topics[i]) + strlen(TOPIC_STATE)+1];
        		snprintf(topic_buf_set, sizeof(topic_buf_set), "%s%s"TOPIC_CMD, config.mqtt_prefix, config.mqtt_topics[i]);
        		snprintf(topic_buf_state, sizeof(topic_buf_state), "%s%s"TOPIC_STATE, config.mqtt_prefix, config.mqtt_topics[i]);
           		if(!strcmp(topic, topic_buf_set)){
           			if(!strcmp(data, "ON")){
           				gpio_set_level(outputs[i], ON_LEVEL);
           				ESP_LOGI(TAG, "Publishing state ON to %s", topic_buf_state);
           				esp_mqtt_client_publish(client, topic_buf_state, "ON", 0, 1, 1);
           				notify_state_mesh(i, 1);
           			} else if(!strcmp(data, "OFF")){
           				gpio_set_level(outputs[i], OFF_LEVEL);
           				ESP_LOGI(TAG, "Publishing state OFF to %s", topic_buf_state);
           				esp_mqtt_client_publish(client, topic_buf_state, "OFF", 0, 1, 1);
           				notify_state_mesh(i, 0);
           			}
            	}
            }
            free(topic);
            free(data);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
    return ESP_OK;
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    mqtt_event_handler_cb(event_data);
}

void mqtt_init(){
	esp_mqtt_client_config_t mqtt_cfg = {};
	mqtt_cfg.uri = malloc(strlen(config.mqtt_broker) + 8);
	strcpy(mqtt_cfg.uri, "mqtt://");
	strcat(mqtt_cfg.uri, config.mqtt_broker);
	client = esp_mqtt_client_init(&mqtt_cfg);
	esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
	esp_mqtt_client_start(client);
}

void notify_state_mqtt(uint8_t output, uint8_t value){
	if(client && config.mqtt_state){
		char topic_buf_state[strlen(config.mqtt_prefix) + strlen(config.mqtt_topics[output]) + strlen(TOPIC_STATE)+1];
		snprintf(topic_buf_state, sizeof(topic_buf_state), "%s%s"TOPIC_STATE, config.mqtt_prefix,config.mqtt_topics[output]);
		if (value) esp_mqtt_client_publish(client, topic_buf_state, "ON", 0, 1, 1);
		else esp_mqtt_client_publish(client, topic_buf_state, "OFF", 0, 1, 1);

	}
}

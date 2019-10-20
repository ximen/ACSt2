/*
 * httpdpart.c
 *
 *  Created on: 16 июл. 2019 г.
 *      Author: ximen
 */
#include "esp_log.h"
#include "esp_event.h"
#include <esp_http_server.h>
#include <stdio.h>
#include "nvs_config.h"
#include "board.h"
#include <ctype.h>

#define TAG "HTTPD_PART"

extern x_config_t config;
const char* part1 = "<html><head><style>form{font:1em sans-serif;max-width:100%;}"
			"p > label{display: block;}"
			"input[type=text],input[type=password],fieldset{width:100%;border:1px solid #333;box-sizing: border-box;}</style>"
			"</head><body style='width: 90%; max-width: 1024px;margin-right: auto;margin-left: auto;background-color:#E6E6FA'><form action='/set' method='get'><p>"
			"<fieldset><legend>Wi-Fi settings <abbr title='This field is mandatory'>*</abbr></legend>"
			"<label for='ssid'>SSID:</label><input type='text' name='ssid' required value='";
const char* part2 = "'><br><label for='pass'>WPA/PSK key:</label><input type='password' name='pass' required value='";
const char* part3 = "'></fieldset></p>"
			"<p><fieldset><legend>Protocols</legend><input type='checkbox' name='bt_state' value='1' ";
const char* part4 = "><label for='bt_state'>Enable Bluetooth Mesh</label><br>"
			"<input type='checkbox' name='mqtt_state' value='1'";
const char* part5 = "><label for='bt_state'>Enable MQTT</label><br>"
			"<label for='mqtt_broker'>MQTT Broker:</label><input type='text' name='mqtt_broker' value='";
const char* part6 = "'></fieldset></p><p><fieldset><legend>Channel topics</legend><label for='topic1'>Topic prefix 1</label><input type='text' name='topic1' value='";
const char* part7 = "'><br><label for='topic2'>Topic prefix 2</label><input type='text' name='topic2' value='";
//const char* part8 = "'><br><label for='topic3'>Topic prefix 3</label><input type='text' name='topic3' value='";
//const char* part9 = "'><br><label for='topic4'>Topic prefix 4</label><input type='text' name='topic4' value='";
//const char* part10 = "'><br><label for='topic5'>Topic prefix 5</label><input type='text' name='topic5' value='";
//const char* part11 = "'><br><label for='topic6'>Topic prefix 6</label><input type='text' name='topic6' value='";
//const char* part12 = "'><br><label for='topic7'>Topic prefix 7</label><input type='text' name='topic7' value='";
//const char* part13 = "'><br><label for='topic8'>Topic prefix 8</label><input type='text' name='topic8' value='";
//const char* part14 = "'><br><label for='topic9'>Topic prefix 9</label><input type='text' name='topic9' value='";
//const char* part15 = "'><br><label for='topic10'>Topic prefix 10</label><input type='text' name='topic10' value='";
const char* part16 = "'><br></fieldset></p><label for='mqtt_prefix'>MQTT prefix:</label><input type='text' name='mqtt_prefix' value='";
const char* part17 = "'><br><p align='center'><input type='submit' value='Submit'></p></form></body></html>";

static esp_err_t config_get_handler(httpd_req_t *req){
	char* resp_str;
	resp_str = malloc(strlen(part1) + strlen(config.ssid) + strlen(part2) + strlen(config.pass) + strlen(part3) + 7 + strlen(part4) + 7 + strlen(part5) +
			strlen(config.mqtt_broker) + strlen(part6) + strlen(config.mqtt_topics[0]) + strlen(part7) +
//			strlen(config.mqtt_topics[1]) + strlen(part8) + strlen(config.mqtt_topics[2]) + strlen(part9) +
//			strlen(config.mqtt_topics[3]) + strlen(part10) + strlen(config.mqtt_topics[4]) + strlen(part11) +
//			strlen(config.mqtt_topics[5]) + strlen(part12) + strlen(config.mqtt_topics[6]) + strlen(part13) +
//			strlen(config.mqtt_topics[7]) + strlen(part14) + strlen(config.mqtt_topics[8]) + strlen(part15) +
			strlen(config.mqtt_topics[1]) +strlen(part16) + strlen(config.mqtt_prefix) + strlen(part17) + 1);
	strcpy(resp_str, part1);
	strcat(resp_str, config.ssid);
	strcat(resp_str, part2);
	strcat(resp_str, config.pass);
	strcat(resp_str, part3);
	if(config.bt_state) strcat(resp_str, "checked");
	strcat(resp_str, part4);
	if(config.mqtt_state) strcat(resp_str, "checked");
	strcat(resp_str, part5);
	strcat(resp_str, config.mqtt_broker);
	strcat(resp_str, part6);
	strcat(resp_str, config.mqtt_topics[0]);
	strcat(resp_str, part7);
	strcat(resp_str, config.mqtt_topics[1]);
//	strcat(resp_str, part8);
//	strcat(resp_str, config.mqtt_topics[2]);
//	strcat(resp_str, part9);
//	strcat(resp_str, config.mqtt_topics[3]);
//	strcat(resp_str, part10);
//	strcat(resp_str, config.mqtt_topics[4]);
//	strcat(resp_str, part11);
//	strcat(resp_str, config.mqtt_topics[5]);
//	strcat(resp_str, part12);
//	strcat(resp_str, config.mqtt_topics[6]);
//	strcat(resp_str, part13);
//	strcat(resp_str, config.mqtt_topics[7]);
//	strcat(resp_str, part14);
//	strcat(resp_str, config.mqtt_topics[8]);
//	strcat(resp_str, part15);
//	strcat(resp_str, config.mqtt_topics[9]);
	strcat(resp_str, part16);
	strcat(resp_str, config.mqtt_prefix);
	strcat(resp_str, part17);
	httpd_resp_send(req, resp_str, strlen(resp_str));
	free(resp_str);
	return ESP_OK;
}

char *urlDecode(const char *str) {
  char *dStr = (char *) malloc(strlen(str) + 1);
  char eStr[] = "00"; /* for a hex code */
  strcpy(dStr, str);
    int i; /* the counter for the string */
    for(i=0;i<strlen(dStr);++i) {

      if(dStr[i] == '%') {
        if(dStr[i+1] == 0)
          return dStr;
        if(isxdigit(dStr[i+1]) && isxdigit(dStr[i+2])) {
          /* combine the next to numbers into one */
          eStr[0] = dStr[i+1];
          eStr[1] = dStr[i+2];
          /* convert it to decimal */
          long int x = strtol(eStr, NULL, 16);
          /* remove the hex */
          memmove(&dStr[i+1], &dStr[i+3], strlen(&dStr[i+3])+1);
          dStr[i] = x;
        }
      }
      else if(dStr[i] == '+') { dStr[i] = ' '; }
    }
  return dStr;
}

static esp_err_t config_set_handler(httpd_req_t *req){
	char*  buf;
	size_t buf_len;
	esp_err_t err;

	buf_len = httpd_req_get_url_query_len(req) + 1;
	if (buf_len > 1) {
		buf = malloc(buf_len);
	    if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
	    	char param[64];
	    	err = httpd_query_key_value(buf, "ssid", param, sizeof(param));
	    	if(err != ESP_OK){
	    		ESP_LOGE(TAG, "Error getting SSID data: %s\n", esp_err_to_name(err));
	    		free(buf);
	    		return err;
	    	}
	    	strlcpy(config.ssid, param, strlen(&param[0]) + 1);
	    	err = httpd_query_key_value(buf, "pass", param, sizeof(param));
	    	if(err != ESP_OK){
	    		ESP_LOGE(TAG, "Error getting pass data: %s\n", esp_err_to_name(err));
	    		free(buf);
	    		return err;
	    	}
	    	strlcpy(config.pass, param, strlen(&param[0]) + 1);
	    	err = httpd_query_key_value(buf, "bt_state", param, sizeof(param));
	    	if(err != ESP_OK){
	    		ESP_LOGE(TAG, "Error getting BT state data: %s. Setting off\n", esp_err_to_name(err));
	    		param[0] = 48;
	    	}
	    	config.bt_state = param[0] - 48;
	    	err = httpd_query_key_value(buf, "mqtt_state", param, sizeof(param));
	    	if(err != ESP_OK){
	    		ESP_LOGE(TAG, "Error getting MQTT state data: %s. Setting off\n", esp_err_to_name(err));
	    		param[0] = 48;
	    	}
	    	config.mqtt_state = param[0] - 48;
	    	err = httpd_query_key_value(buf, "mqtt_broker", param, sizeof(param));
	    	if(err != ESP_OK){
	    		ESP_LOGE(TAG, "Error getting MQTT broker address: %s. Setting null\n", esp_err_to_name(err));
	    		param[0] = 0;
	    	}
	    	strlcpy(config.mqtt_broker, param, strlen(&param[0]) + 1);
	    	err = httpd_query_key_value(buf, "mqtt_prefix", param, sizeof(param));
	    	if(err != ESP_OK){
	    		ESP_LOGE(TAG, "Error getting MQTT prefix: %s. Setting null\n", esp_err_to_name(err));
	    		param[0] = 0;
	    	}
	    	strlcpy(config.mqtt_prefix, urlDecode(param), strlen(&param[0]) + 1);
	    	char topic_buf[8];
	    	for(uint8_t i = 0; i < OUTPUT_NUM; i++){
	    		snprintf(topic_buf, 8, "topic%d", i+1);
	    		err = httpd_query_key_value(buf, &topic_buf[0], param, sizeof(param));
	    		if(err == ESP_OK){
	    			strlcpy(config.mqtt_topics[i], urlDecode(param), strlen(&param[0]) + 1);
	    			ESP_LOGI(TAG, "Got topic %d: %s <- %s\n", i + 1, config.mqtt_topics[i], &param[0]);
	    		} else {
	    			ESP_LOGE(TAG, "Topic %d not specified.\n", i + 1);
	    		}
	    	}
	    	print_conf();
	    	if ((strlen(config.ssid) < 2)||(strlen(config.ssid) > 32)||(strlen(config.pass) < 8)) config.wifi_mode = WIFI_MODE_AP; else config.wifi_mode = WIFI_MODE_STA;
	    }
	    free(buf);
	    err = save_config();
    	if(err != ESP_OK){
    		ESP_LOGE(TAG, "Error saving config: %s\n", esp_err_to_name(err));
    		return err;
    	}
	}

	const char* resp_str = (const char*) req->user_ctx;
	httpd_resp_send(req, resp_str, strlen(resp_str));
	return ESP_OK;
}

static const httpd_uri_t config_uri = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = config_get_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
};

static const httpd_uri_t set_uri = {
    .uri       = "/set",
    .method    = HTTP_GET,
    .handler   = config_set_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
	.user_ctx  = "Hello",
};

void stop_webserver(httpd_handle_t server){
    // Stop the httpd server
    httpd_stop(server);
}


httpd_handle_t start_webserver(void){
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &config_uri);
        httpd_register_uri_handler(server, &set_uri);
        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}

static void disconnect_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data){
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server) {
        ESP_LOGI(TAG, "Stopping webserver");
        stop_webserver(*server);
        *server = NULL;
    }
}

static void connect_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data){
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server == NULL) {
        ESP_LOGI(TAG, "Starting webserver");
        *server = start_webserver();
    }
}

/*
 * btpart.c
 *
 *  Created on: 16 июл. 2019 г.
 *      Author: ximen
 */
#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_event.h"
#include "esp_err.h"

#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"

#include "model_opcode.h"
#include "esp_ble_mesh_defs.h"
#include "esp_ble_mesh_common_api.h"
#include "esp_ble_mesh_networking_api.h"
#include "esp_ble_mesh_provisioning_api.h"
#include "esp_ble_mesh_config_model_api.h"
#include "esp_ble_mesh_generic_model_api.h"
#include "transport.h"

#include "board.h"
#include "btpart.h"
#include "mqttpart.h"
#include "nvs_config.h"

extern x_config_t config;

#define TAG "BT_PART"

//extern rx_cb_t ble_transport_rx_cb;

extern gpio_num_t outputs[OUTPUT_NUM];
struct k_delayed_work init_timer;
struct k_delayed_work unprov_timer;

static uint8_t dev_uuid[16] = { 0xdd, 0xdd };
static uint8_t prov_start_num = 0;
static bool prov_start = false;

/* Configuration Client Model user_data */
esp_ble_mesh_client_t config_client;

/* Configuration Server Model user_data */
esp_ble_mesh_cfg_srv_t config_server = {
    .relay = ESP_BLE_MESH_RELAY_ENABLED,
    .beacon = ESP_BLE_MESH_BEACON_DISABLED,
#if defined(CONFIG_BLE_MESH_FRIEND)
    .friend_state = ESP_BLE_MESH_FRIEND_ENABLED,
#else
    .friend_state = ESP_BLE_MESH_FRIEND_NOT_SUPPORTED,
#endif
#if defined(CONFIG_BLE_MESH_GATT_PROXY)
    .gatt_proxy = ESP_BLE_MESH_GATT_PROXY_ENABLED,
#else
    .gatt_proxy = ESP_BLE_MESH_GATT_PROXY_NOT_SUPPORTED,
#endif
    .default_ttl = 7,
    /* 3 transmissions with 20ms interval */
    .net_transmit = ESP_BLE_MESH_TRANSMIT(2, 20),
    .relay_retransmit = ESP_BLE_MESH_TRANSMIT(2, 20),
};

static esp_ble_mesh_model_op_t onoff_op[] = {
    ESP_BLE_MESH_MODEL_OP(ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_GET,       0),
    ESP_BLE_MESH_MODEL_OP(ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET,       2),
    ESP_BLE_MESH_MODEL_OP(ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET_UNACK, 2),
    ESP_BLE_MESH_MODEL_OP_END,
};

ESP_BLE_MESH_MODEL_PUB_DEFINE(onoff_pub, 2 + 1, ROLE_NODE);

static esp_ble_mesh_model_t root_models[] = {
    ESP_BLE_MESH_MODEL_CFG_SRV(&config_server),
};

static esp_ble_mesh_model_t onoff_models[OUTPUT_NUM][1] = {
		{{.model_id = ESP_BLE_MESH_MODEL_ID_GEN_ONOFF_SRV, .op = onoff_op, .pub = &onoff_pub, 					\
				.keys = { [0 ... (CONFIG_BLE_MESH_MODEL_KEY_COUNT - 1)] = ESP_BLE_MESH_KEY_UNUSED }, 			\
				.groups = { [0 ... (CONFIG_BLE_MESH_MODEL_GROUP_COUNT - 1)] = ESP_BLE_MESH_ADDR_UNASSIGNED }, 	\
				.user_data = &outputs[0]}},
		{{.model_id = ESP_BLE_MESH_MODEL_ID_GEN_ONOFF_SRV, .op = onoff_op, .pub = &onoff_pub, 					\
				.keys = { [0 ... (CONFIG_BLE_MESH_MODEL_KEY_COUNT - 1)] = ESP_BLE_MESH_KEY_UNUSED }, 			\
				.groups = { [0 ... (CONFIG_BLE_MESH_MODEL_GROUP_COUNT - 1)] = ESP_BLE_MESH_ADDR_UNASSIGNED }, 	\
				.user_data = &outputs[1]}},
};

static esp_ble_mesh_elem_t elements[] = {
    ESP_BLE_MESH_ELEMENT(0, root_models, ESP_BLE_MESH_MODEL_NONE),
	ESP_BLE_MESH_ELEMENT(0, onoff_models[0], ESP_BLE_MESH_MODEL_NONE),
	ESP_BLE_MESH_ELEMENT(0, onoff_models[1], ESP_BLE_MESH_MODEL_NONE),
};

static esp_ble_mesh_comp_t comp = {
    .cid           = CID_ESP,
    .elements      = elements,
    .element_count = ARRAY_SIZE(elements),
};

static esp_ble_mesh_prov_t prov = {
    .uuid                = dev_uuid,
    .output_size         = 0,
    .output_actions      = 0,
};

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    uint8_t level = gpio_get_level(BUTTON_PIN);
    if (level){
    	k_delayed_work_cancel(&unprov_timer);
    } else {
    	k_delayed_work_submit(&unprov_timer, UNPROVISION_BUTTON_INTERVAL);
    }
}

void rx_cb(u16_t src, u8_t *data, u16_t len){

}

static void gen_onoff_get_handler(esp_ble_mesh_model_t *model, esp_ble_mesh_msg_ctx_t *ctx, uint16_t len, uint8_t *data){
    uint8_t send_data;
    esp_err_t err;

    ESP_LOGI(TAG, "GET Handler");

    gpio_num_t *output = (gpio_num_t *)model->user_data;
    if (!output) {
        ESP_LOGE(TAG, "%s: Failed to get generic onoff server model user_data", __func__);
        return;
    }

    if (gpio_get_level(*output) == ON_LEVEL) send_data = 1; else send_data = 0;

    /* Sends Generic OnOff Status as a reponse to Generic OnOff Get */
    err = esp_ble_mesh_server_model_send_msg(model, ctx, ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_STATUS, sizeof(send_data), &send_data);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "%s: Failed to send Generic OnOff Status message", __func__);
        return;
    }
}

static void gen_onoff_set_unack_handler(esp_ble_mesh_model_t *model, esp_ble_mesh_msg_ctx_t *ctx, uint16_t len, uint8_t *data){
    gpio_num_t *output = NULL;

    output = (gpio_num_t *)model->user_data;

    if (!output) {
        ESP_LOGE(TAG, "%s: Failed to get generic onoff server model user_data", __func__);
        return;
    }

    ESP_LOGI(TAG, "Setting GPIO %d", *output);

    if (data[0] == 0) gpio_set_level(*output, OFF_LEVEL); else gpio_set_level(*output, ON_LEVEL);
    notify_state_mqtt(get_output_by_pin(*output), data[0]);
}

static void gen_onoff_set_handler(esp_ble_mesh_model_t *model, esp_ble_mesh_msg_ctx_t *ctx, uint16_t len, uint8_t *data){
    gen_onoff_set_unack_handler(model, ctx, len, data);
    gen_onoff_get_handler(model, ctx, len, data);
}

static void node_prov_complete(uint16_t net_idx, uint16_t addr, uint8_t flags, uint32_t iv_index){
    ESP_LOGI(TAG, "net_idx: 0x%04x, unicast_addr: 0x%04x", net_idx, addr);
    ESP_LOGI(TAG, "flags: 0x%02x, iv_index: 0x%08x", flags, iv_index);
    board_prov_complete();
}

static void provisioner_prov_link_open(esp_ble_mesh_prov_bearer_t bearer){
    ESP_LOGI(TAG, "%s: bearer %s", __func__, bearer == ESP_BLE_MESH_PROV_ADV ? "PB-ADV" : "PB-GATT");
}

static void provisioner_prov_link_close(esp_ble_mesh_prov_bearer_t bearer, uint8_t reason){
    ESP_LOGI(TAG, "%s: bearer %s, reason 0x%02x", __func__, bearer == ESP_BLE_MESH_PROV_ADV ? "PB-ADV" : "PB-GATT", reason);
    if (prov_start_num) prov_start_num--;
}

static void example_ble_mesh_provisioning_cb(esp_ble_mesh_prov_cb_event_t event, esp_ble_mesh_prov_cb_param_t *param){
    switch (event) {
    case ESP_BLE_MESH_PROV_REGISTER_COMP_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_PROV_REGISTER_COMP_EVT, err_code: %d",
                 param->prov_register_comp.err_code);
        break;
    case ESP_BLE_MESH_NODE_PROV_ENABLE_COMP_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_NODE_PROV_ENABLE_COMP_EVT, err_code: %d",
                 param->node_prov_enable_comp.err_code);
        break;
    case ESP_BLE_MESH_NODE_PROV_LINK_OPEN_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_NODE_PROV_LINK_OPEN_EVT, bearer: %s",
                 param->node_prov_link_open.bearer == ESP_BLE_MESH_PROV_ADV ? "PB-ADV" : "PB-GATT");
        break;
    case ESP_BLE_MESH_NODE_PROV_LINK_CLOSE_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_NODE_PROV_LINK_CLOSE_EVT, bearer: %s",
                 param->node_prov_link_close.bearer == ESP_BLE_MESH_PROV_ADV ? "PB-ADV" : "PB-GATT");
        break;
    case ESP_BLE_MESH_NODE_PROV_COMPLETE_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_NODE_PROV_COMPLETE_EVT");
        node_prov_complete(param->node_prov_complete.net_idx, param->node_prov_complete.addr,
            param->node_prov_complete.flags, param->node_prov_complete.iv_index);
        break;
    case ESP_BLE_MESH_NODE_PROXY_GATT_DISABLE_COMP_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_NODE_PROXY_GATT_DISABLE_COMP_EVT");
        prov_start = true;
        break;
    case ESP_BLE_MESH_PROVISIONER_PROV_LINK_OPEN_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_PROVISIONER_PROV_LINK_OPEN_EVT");
        provisioner_prov_link_open(param->provisioner_prov_link_open.bearer);
        break;
    case ESP_BLE_MESH_PROVISIONER_PROV_LINK_CLOSE_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_PROVISIONER_PROV_LINK_CLOSE_EVT");
        provisioner_prov_link_close(param->provisioner_prov_link_close.bearer,
                                    param->provisioner_prov_link_close.reason);
        break;
    case ESP_BLE_MESH_PROVISIONER_ADD_UNPROV_DEV_COMP_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_PROVISIONER_ADD_UNPROV_DEV_COMP_EVT, err_code: %d",
                 param->provisioner_add_unprov_dev_comp.err_code);
        break;
    case ESP_BLE_MESH_PROVISIONER_SET_DEV_UUID_MATCH_COMP_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_PROVISIONER_SET_DEV_UUID_MATCH_COMP_EVT, err_code: %d",
                 param->provisioner_set_dev_uuid_match_comp.err_code);
        break;
    case ESP_BLE_MESH_PROVISIONER_SET_NODE_NAME_COMP_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_PROVISIONER_SET_NODE_NAME_COMP_EVT, err_code: %d",
                 param->provisioner_set_node_name_comp.err_code);
        break;
    default:
        break;
    }

    return;
}

static void example_ble_mesh_custom_model_cb(esp_ble_mesh_model_cb_event_t event,
        esp_ble_mesh_model_cb_param_t *param)
{
    uint32_t opcode;
    esp_err_t err;

    ESP_LOGI(TAG, "Event %d", event);

    switch (event) {
    case ESP_BLE_MESH_MODEL_OPERATION_EVT: {
        if (!param->model_operation.model || !param->model_operation.model->op || !param->model_operation.ctx) {
            ESP_LOGE(TAG, "%s: model_operation parameter is NULL", __func__);
            return;
        }
        opcode = param->model_operation.opcode;
        switch (opcode) {
        case ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_GET:
        	gen_onoff_get_handler(param->model_operation.model, param->model_operation.ctx,
        						  param->model_operation.length, param->model_operation.msg);
        	break;
        case ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET:
            gen_onoff_set_handler(param->model_operation.model, param->model_operation.ctx,
                                  param->model_operation.length, param->model_operation.msg);
            break;
        case ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET_UNACK:
            gen_onoff_set_unack_handler(param->model_operation.model, param->model_operation.ctx,
                                        param->model_operation.length, param->model_operation.msg);
            break;
        default:
            ESP_LOGI(TAG, "%s: opcode 0x%04x", __func__, param->model_operation.opcode);
            break;
        }
        break;
    }
    case ESP_BLE_MESH_MODEL_SEND_COMP_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_MODEL_SEND_COMP_EVT, err_code %d", param->model_send_comp.err_code);
        break;
    case ESP_BLE_MESH_MODEL_PUBLISH_COMP_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_MODEL_PUBLISH_COMP_EVT, err_code %d", param->model_publish_comp.err_code);
        break;
    case ESP_BLE_MESH_CLIENT_MODEL_RECV_PUBLISH_MSG_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_MODEL_CLIENT_RECV_PUBLISH_MSG_EVT, opcode 0x%04x", param->client_recv_publish_msg.opcode);
        break;
    default:
        break;
    }
}

static void example_ble_mesh_config_server_cb(esp_ble_mesh_cfg_server_cb_event_t event, esp_ble_mesh_cfg_server_cb_param_t *param){
    ESP_LOGI(TAG, "%s, event = 0x%02x, opcode = 0x%04x, addr: 0x%04x",
             __func__, event, param->ctx.recv_op, param->ctx.addr);
}

esp_err_t ble_mesh_init(void)
{
    esp_err_t err;

    /* First two bytes of device uuid is compared with match value by Provisioner */
    memcpy(dev_uuid + 2, esp_bt_dev_get_address(), 6);

    esp_ble_mesh_register_prov_callback(example_ble_mesh_provisioning_cb);
    esp_ble_mesh_register_config_server_callback(example_ble_mesh_config_server_cb);
    esp_ble_mesh_register_custom_model_callback(example_ble_mesh_custom_model_cb);


    err = esp_ble_mesh_init(&prov, &comp);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "%s: Failed to initialize BLE Mesh", __func__);
        return err;
    }

    //k_delayed_work_init(&send_self_prov_node_addr_timer, example_send_self_prov_node_addr);

    err = esp_ble_mesh_node_prov_enable(ESP_BLE_MESH_PROV_ADV | ESP_BLE_MESH_PROV_GATT);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "%s: Failed to enable node provisioning", __func__);
        return err;
    }

    ESP_LOGI(TAG, "Meshtek ACSt initialized");

	gpio_set_level(LED_PIN, 1);
	k_delayed_work_init(&init_timer, led_off);
	k_delayed_work_init(&unprov_timer, timer_stop);
	k_delayed_work_submit(&init_timer, PROVISION_LED_INTERVAL);

    //install gpio isr service
	gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
	//hook isr handler for specific gpio pin
	gpio_isr_handler_add(BUTTON_PIN, gpio_isr_handler, (void*) BUTTON_PIN);

	//ble_transport_rx_cb = rx_cb;
	//ESP_LOGI(TAG, "RX_CB: %d", (int)ble_transport_rx_cb);
    return ESP_OK;
}

esp_err_t bluetooth_init(void)
{
    esp_err_t ret;

    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(TAG, "%s initialize controller failed", __func__);
        return ret;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        ESP_LOGE(TAG, "%s enable controller failed", __func__);
        return ret;
    }

    ret = esp_bluedroid_init();
    if (ret) {
        ESP_LOGE(TAG, "%s init bluetooth failed", __func__);
        return ret;
    }

    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(TAG, "%s enable bluetooth failed", __func__);
        return ret;
    }

    return ret;
}

void notify_state_mesh(uint8_t output, uint8_t value){
	if(config.bt_state){
//		esp_ble_mesh_msg_ctx_t ctx = {
//			    /** NetKey Index of the subnet through which to send the message. */
//			    .net_idx = 1,
//			    /** AppKey Index for message encryption. */
//			    .app_idx = onoff_models[output][0].pub->app_idx,
//			    /** Remote address. */
//			    .addr = onoff_models[output][0].pub->publish_addr,
//			    /** TTL, or BLE_MESH_TTL_DEFAULT for default TTL. */
//			    .send_ttl = BLE_MESH_TTL_DEFAULT,
//		};
		//onoff_models[output][0].pub->msg->data[0] = value;
		esp_ble_mesh_model_publish(&onoff_models[output][0], ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_STATUS, sizeof(value), &value, ROLE_NODE);

		//gen_onoff_get_handler(onoff_models[output], esp_ble_mesh_msg_ctx_t *ctx, uint16_t len, uint8_t *data);
	}
}

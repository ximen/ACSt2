/*
 * btpart.h
 *
 *  Created on: 16 июл. 2019 г.
 *      Author: ximen
 */

#ifndef MAIN_BTPART_H_
#define MAIN_BTPART_H_

esp_err_t bluetooth_init(void);
esp_err_t ble_mesh_init(void);
void notify_state_mesh(uint8_t output, uint8_t value);

#endif /* MAIN_BTPART_H_ */

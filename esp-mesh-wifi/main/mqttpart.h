/*
 * mqttpart.h
 *
 *  Created on: 30 июл. 2019 г.
 *      Author: ximen
 */

#ifndef MAIN_MQTTPART_H_
#define MAIN_MQTTPART_H_

void mqtt_init();
void notify_state_mqtt(uint8_t output, uint8_t value);

#endif /* MAIN_MQTTPART_H_ */

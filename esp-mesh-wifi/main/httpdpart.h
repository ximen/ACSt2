/*
 * httpdpart.h
 *
 *  Created on: 16 июл. 2019 г.
 *      Author: ximen
 */

#ifndef MAIN_HTTPDPART_H_
#define MAIN_HTTPDPART_H_

#include <esp_http_server.h>

httpd_handle_t start_webserver(void);
void stop_webserver(httpd_handle_t server);

#endif /* MAIN_HTTPDPART_H_ */

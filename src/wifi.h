#ifndef WIFI_H_
#define WIFI_H_
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "esp_http_client.h"
#include <stdio.h>
#include <string.h>
//My Password Data
#include "/home/david/.confidential/HomeWAP.h"

#define WIFI_IP_UP              0
#define WIFI_IP_UNASSIGNED      -1
#define WIFI_INT_UP             1
#define WIFI_INT_DOWN           2
#define WIFI_ERROR              -2

#define HTTP_URL_BUFF_SIZE 256

esp_err_t   wifiInit();
int         wifiStatus();

esp_err_t   httpGet(const char* url);
esp_err_t   httpGetBuffer(const char* url,char* buffer, size_t buffersize);
#endif
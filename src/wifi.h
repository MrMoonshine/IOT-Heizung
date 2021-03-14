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

esp_err_t wifiInit();
void httptest();
#endif
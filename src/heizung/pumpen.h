#ifndef PUMPEN_H
#define PUMPEN_H
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"

#include "driver/gpio.h"
#include "../wifi.h"
#include "webserver/URLs.h"
#include <stdbool.h>
/*
    Der zustand von allen Pumpen wird im NVS flash gespeichert.
    Die Realys vom 4-Wege-Mischer werden nicht gespeichert.
*/
typedef struct{
    gpio_num_t gpio;
    int8_t mask;
}Pumpe;

//Relay boards sind invertiert
#define PUMP_ON     0
#define PUMP_OFF    1
#define PUMP_ERR    -1
#define PUMPENANZAHL 6
#define PUMP_SOLAR_GPIO 2

/*-------------Functions-------------------*/
esp_err_t pumpsInit();
/**
 * @brief  Diese Funktion Fragt alle zuständer der Pumpen vom Webserver ab.
 * @return zustände aller pumpen als bitmuster. oder -1 bei fehlern
*/
int8_t pumpsSync();
/**
 * @brief  Diese Funktion Fragt alle zuständer der Pumpen von den GPIO ports ab.
 * @return zustände aller pumpen als bitmuster. oder -1 bei fehlern.
*/
int8_t pumpsRead();
/**
 * @brief  Diese Funktion schreibt das bitmuster auf die GPIO pins
 * @param[in] states Zustände der Pumpen
 * @return 
 * - ESP_OK on success 
 * - ESP_FAIL on error
*/
esp_err_t pumpsWrite(int8_t states);
#endif
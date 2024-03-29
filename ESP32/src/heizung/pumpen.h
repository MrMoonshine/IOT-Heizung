#ifndef PUMPEN_H
#define PUMPEN_H
#include <rest.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"

#include "driver/gpio.h"
#include "../wifi.h"
#include <stdbool.h>

typedef int8_t pump_states_t;
/*
    Der zustand von allen Pumpen wird im NVS flash gespeichert.
    Die Realys vom 4-Wege-Mischer werden nicht gespeichert.
*/
typedef struct{
    const char name[24];
    gpio_num_t gpio;
    pump_states_t mask;
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
 * @brief  Diese Funktion Fragt alle zuständer der Pumpen von den GPIO ports ab.
 * @return zustände aller pumpen als bitmuster. oder -1 bei fehlern.
*/
pump_states_t pumpsRead();
/**
 * @brief  Ausgangszustände der Pumpen. (für restart).
 * @return zustände aller pumpen als bitmuster. oder -1 bei fehlern.
*/
pump_states_t pumpsDefault();
/**
 * @brief  Diese Funktion schreibt das bitmuster auf die GPIO pins
 * @param[in] states Zustände der Pumpen
 * @return 
 * - ESP_OK on success 
 * - ESP_FAIL on error
*/
esp_err_t pumpsWrite(pump_states_t states);
/*
    @brief API Resource um Pumpen zu Steuern
    @param[in] req HTTP request
*/
esp_err_t heizung_api_pumps(httpd_req_t *req);
#endif
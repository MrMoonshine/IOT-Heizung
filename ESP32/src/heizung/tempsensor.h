#pragma once
#include <string.h>
#include <float.h>

#include <esp_http_server.h>
#include <rest.h>
#include <include/owb.h>
#include <include/owb_rmt.h>
#include <include/ds18b20.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "driver/adc.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_continuous.h"
#include "esp_system.h"
#include "esp_log.h"
#include "math.h"
#include "romcodes.h"

// 50 Sekunden
#define TEMPSENSOR_READ_INTERVAL 70
// Max length of a sensor name
#define TEMPSENSOR_NAME_LEN 8

typedef struct heizung_temperatur_t{
    char name[8];
    uint8_t id;
    OneWireBus_ROMCode* romcode;
    DS18B20_Info info;
    float value;
    // Next element
    struct heizung_temperatur_t *next;
}heizung_temperatur_t;

#define DS_RESOLUTION DS18B20_RESOLUTION_12_BIT
#define ZERO_KELVIN -273.15
#define TEMPERATURE_FAIL -2048
/*
    @brief Linked List mit Temperaturen, namen und messwerten 
    @parameter[in] head Liste
    @parameter[in] bus One Wire Bus
    @parameter[in] name Name des Sensors im JSON
*/
esp_err_t temp_owb_add_sensor(heizung_temperatur_t** head, OneWireBus *bus, OneWireBus_ROMCode *code, uint8_t id, const char* name);
/*
    @brief Erstellt alle Sensoren
*/
esp_err_t temp_owb_add_all(OneWireBus *bus);
/*
    @brief Alle OWB Sensoren Lesen, und Messwerte speichern
*/
esp_err_t temp_owb_read_sensors();
/*
    @brief Alle OWB Sensoren Zählen
*/
unsigned int temp_owb_count_sensors();
/*
    @brief Pointer Weitergabe
    @returns Pointer zur Liste
*/
heizung_temperatur_t* temp_owb_list();
//alter the settings to fit the sensors
void tempDoSettings(OneWireBus *owb);
/*
    @brief Analogen Sensor Lesen (Zum Glück gibts da nur die Solar ;) )
*/
void temp_analog_init();
/*
    @brief Analogen Sensor Lesen (Zum Glück gibts da nur die Solar ;) )
    @param v Spannung am ADC in mV. NULL = ingnore
    @param Rt Wiederstand am ADC. NULL = ingnore
    @return Temperatur in °C
*/
float temp_analog_read(uint32_t* v_i, int* Rt_i);
/*
    @brief API Callback
*/
esp_err_t heizung_api_temperatures(httpd_req_t *req);
esp_err_t heizung_api_ntc(httpd_req_t *req);
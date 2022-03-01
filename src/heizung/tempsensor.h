#pragma once
#include <include/owb.h>
#include <include/owb_rmt.h>
#include <include/ds18b20.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h"
#include "driver/gpio.h"
#include "esp_adc_cal.h"
#include "esp_system.h"
#include "esp_log.h"
#include "math.h"
#include "string.h"
#include "romcodes.h"
#include "webserver/URLs.h"

#define DS_RESOLUTION DS18B20_RESOLUTION_12_BIT
#define ZERO_KELVIN -273.15
#define TEMPERATURE_FAIL -2048

//Does an ESP error log on failure
esp_err_t tempBuildSensPtr(OneWireBus *bus, DS18B20_Info *sensors);
bool tempVerify(OneWireBus *bus, OneWireBus_ROMCode *dev);
//alter the settings to fit the sensors
void tempDoSettings(OneWireBus *owb);
//Init the sensor itself
esp_err_t tempInitSensor(OneWireBus *bus, OneWireBus_ROMCode *code, DS18B20_Info *info);
//read Analog Temperature
esp_err_t tempAnalogCalc(int Rt,float *tempp);
int tempGetRt();
void tempAnalogInit();
/*
 * @brief Lesen der Temperatur eines Sensors
 * @param info Der Info Struct vom Sensor
 * @return Temperatur in °C. Bei Fehler kommt -2048°C
*/
float tempReadSensor(DS18B20_Info *info);
/*
 * @brief Nutzt mehrfach die funktion tempReadSensor(). Alle werte kommen in ein float array
 * @param temps float array. da kommen die Ergebnisse rein
 * @param sensors Sensor infos
 * @return ESP_OK. Ausser Solarmessung hatte fehler
*/
esp_err_t tempReadArray(float* temps, DS18B20_Info *sensors);
/*
 * @brief Baut aus einem Float array eine URL für den Webserver
 * @param temps float array. da kommen die daten rein
 * @param url URL buffer
 * @return ESP_OK. Falls keine Korrekte Temperatur existiert dann ESP_
*/
esp_err_t tempArray2URL(float* temps, char* url);
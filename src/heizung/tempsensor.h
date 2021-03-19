#ifndef TEMPSENSOR_H_
#define TEMPSENSOR_H_
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

#define DS_RESOLUTION DS18B20_RESOLUTION_12_BIT
#define TEMP_URL "http://alpakagott/dumpster.php"
#define ZERO_KELVIN -273.15

//Das Array Mit allen Drinnen ist ein Problemfall.
//issue opened
/*typedef struct{
    DS18B20_Info info;                      //Actual Sensor data
    const char name[16];                  //The name on the SQL database
    const OneWireBus_ROMCode *romcode;    //Romcode
} Temperature;

static Temperature roomTemp = {
    .name = "room",
    .romcode = &roomrom,
};

static Temperature redTemp = {
    .name = "red",
    .romcode = &redrom,
};

static Temperature greenTemp = {
    .name = "green",
    .romcode = &greenrom,
};

static Temperature blueTemp = {
    .name = "blue",
    .romcode = &bluerom,
};

static Temperature whiteTemp = {
    .name = "white",
    .romcode = &whiterom,
};

static Temperature yellowTemp = {
    .name = "yellow",
    .romcode = &yellowrom,
};

static Temperature brownTemp = {
    .name = "brown",
    .romcode = &brownrom,
};

static Temperature debug1Temp = {
    .name = "debug1",
    .romcode = &debugtemp1rom,
};

static Temperature debug2Temp = {
    .name = "debug2",
    .romcode = &debugtemp2rom,
};*/

static const char *sensornames[SENSORS_TOTAL + 1] = {
    "room",
    "red",
    "green",
    "blue",
    "white",
    "yellow",
    "brown",
    "debug1",
    "debug2"
};

bool tempVerify(OneWireBus *bus,OneWireBus_ROMCode *dev);
//alter the settings to fit the sensors
void tempDoSettings(OneWireBus *owb);
//Init the sensor itself
esp_err_t tempInitSensor(OneWireBus *bus, OneWireBus_ROMCode *code, DS18B20_Info *info);
esp_err_t tempReadSensor(OneWireBus *bus, DS18B20_Info *info, float *temperature);
//read Analog Temperature
esp_err_t tempAnalogPolynom(int Rt,float *tempp);
int tempGetRt();
void tempAnalogInit();
/*
    * @brief Alle Sensoren In Richtiger Reihenfolge Starten
    * @param bus One-Wire Bus pointer
    * @param sensors Array Aller Info-Name-Structs
    * @return Errors wenn es welche gibt
*/
esp_err_t tempBuildSensPtr(OneWireBus *bus, DS18B20_Info *sensors, unsigned short *sensseq);
/*
    * @brief Alle Sensoren Lesen und als HTTP-GET Request zum server senden
    * @param bus One-Wire Bus pointer
    * @param sensors Array Aller Info-Name-Structs
    * @param temps Array mit allen temperaturen
    * @param url in diesen buffer kommt die fertige URL mit allen GET daten rein
    * @return Errors wenn es welche gibt
*/
esp_err_t tempReadAll(OneWireBus *bus, DS18B20_Info *sensors, float *temps, char *url, unsigned short sensseq);
#endif
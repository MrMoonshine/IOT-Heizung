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
typedef struct{
    DS18B20_Info info;              //Actual Sensor data
    char name[16];                  //The name on the SQL database
    OneWireBus *bus;                //Bus pointer
    const OneWireBus_ROMCode *romcode;    //Romcode
} Temperature;

//Does an ESP error log on failure
esp_err_t tempBuildSensPtr(OneWireBus *bus,Temperature *temps);
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
//Finally Read All
esp_err_t tempReadAll(float* temps, char* url, Temperature *sensors);
#endif
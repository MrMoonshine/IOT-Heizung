#ifndef TEMPSENSOR_H_
#define TEMPSENSOR_H_
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h"
#include "driver/gpio.h"
#include "esp_adc_cal.h"
#include "esp_system.h"
#include "esp_log.h"
#include "math.h"
#include "temeratureSensor.h"
#include "tempdef.h"
#define DS_RESOLUTION DS18B20_RESOLUTION_12_BIT
//Does an ESP error log on failure
bool tempVerify(OneWireBus *bus,OneWireBus_ROMCode *dev);
//Init Bus
OneWireBus* tempInitBus(gpio_num_t pin, owb_rmt_driver_info *rmtDriverInfo);
//alter the settings to fit the sensors
void tempDoSettings(OneWireBus *owb);
//Init the sensor itself
esp_err_t tempInitSensor(OneWireBus *bus, OneWireBus_ROMCode *code, DS18B20_Info *info);
esp_err_t tempReadSensor(OneWireBus *bus, DS18B20_Info *info, float *temperature);
//read Analog Temperature
esp_err_t tempAnalogPolynom(int Rt);
int tempGetRt();
void tempAnalogInit();
#endif
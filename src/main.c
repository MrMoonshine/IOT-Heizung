#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"

#include "driver/gpio.h"
#include "sdkconfig.h"

#include "wifi.h"
#include <string.h>

#include "heizung/tempsensor.h"
#include "heizung/pumpen.h"
#include "heizung/romcodes.h"
#include "core/timer.h"

#define GPIO_ONE_WIRE TEMPERATURE_GPIO
#define BLINK_GPIO CHIPLED
#define MAX_DEVICES 11
#define DS18B20_ROM_CODE_LENGTH 17

static const char* TAG = "Heizung";

//Variables for One Wire Bus
Temperature dssensors[SENSORS_TOTAL];
owb_rmt_driver_info rmtDriverInfo;
OneWireBus *owb;
//Array for all temperatures (+1 beacuse of the analog solar meassure)
float temperatures[SENSORS_TOTAL + 1];

void heatact(void *args){
/*---------------------------------------------------------*/
/*              Temperaturen Messen                        */
/*---------------------------------------------------------*/
    ESP_LOGI(TAG,"Messungen werden durchgeführt\n");
    char url[128] = "";
    ESP_ERROR_CHECK(tempReadAll(temperatures,url));
    httpGet((const char*)url);
}

void app_main(){
    gpio_reset_pin(BLINK_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
    printf("\033[1;31m[E] Alpaka %d\n\033[0m",heizpumpe.gpio);
    ESP_LOGI(TAG, "Starting One-Wire Bus...");
    owb = tempInitBus(GPIO_ONE_WIRE, &rmtDriverInfo);

    tempDoSettings(owb);
    //Init All Sensors
    ESP_ERROR_CHECK(tempBuildSensPtr(owb,dssensors));
    //Init Analog Temperature Meassurement
    tempAnalogInit();
    wifiInit();

    timerInit(&heatact);
    while(1){
        gpio_set_level(BLINK_GPIO, 0);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        /* Blink on (output high) */
        gpio_set_level(BLINK_GPIO, 1);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }    
}
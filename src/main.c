#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"

#include "driver/gpio.h"
#include "sdkconfig.h"

#include "temeratureSensor.h"
#include "tempsensor.h"

#include "wifi.h"
#include "tempdef.h"
#include <string.h>

#include "heizung/pumpen.h"

#define GPIO_ONE_WIRE TEMPERATURE_GPIO
#define BLINK_GPIO CHIPLED
#define MAX_DEVICES 2
#define DS18B20_ROM_CODE_LENGTH 17

static const char* TAG = "Heizung";

//sensor 0: 4600000bc7446e28
OneWireBus_ROMCode sen0romc = {
    .fields.family = {0x28},
    .fields.serial_number = {0x6e,0x44,0xc7,0x0b,0x00,0x00},
    .fields.crc = {0x46}
};
//sensor 1: ea00000bc9940128
OneWireBus_ROMCode sen1romc = {
    .fields.family = {0x28},
    .fields.serial_number = {0x01,0x94,0xc9,0x0b,0x00,0x00},
    .fields.crc = {0xea}
};

void app_main(){
    gpio_reset_pin(BLINK_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
    printf("\033[1;31m[E] Alpaka %d\n\033[0m",heizpumpe.gpio);
    ESP_LOGI(TAG, "Starting One-Wire Bus...");

    owb_rmt_driver_info rmtDriverInfo;
    OneWireBus *owb;
    
    owb = tempInitBus(GPIO_ONE_WIRE, &rmtDriverInfo);
    //owb_use_crc(owb, true);
    tempDoSettings(owb);

    DS18B20_Info mysensor0, mysensor1;
    tempInitSensor(owb,&sen0romc,&mysensor0);
    tempInitSensor(owb,&sen1romc,&mysensor1);

    //Init Analog Temperature Meassurement
    tempAnalogInit();

    wifiInit();

    while(1){
        //httptest();
        printf("Meassuring all Sensors\n");

        float temp = -273;
        tempReadSensor(owb,&mysensor0,&temp);
        printf("Sensor[0]; T = %f°C\n",temp);
        tempReadSensor(owb,&mysensor1,&temp);
        printf("Sensor[1]; T = %f°C\n",temp);

        gpio_set_level(BLINK_GPIO, 0);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        /* Blink on (output high) */
	    //printf("Turning on the LED\n");
        gpio_set_level(BLINK_GPIO, 1);
        tempAnalogPolynom(tempGetRt());
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }    
}
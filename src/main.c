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
/*---------------------------------------------------------*/
/*              Variables for One Wire Bus                 */
/*---------------------------------------------------------*/
Temperature dssensors[SENSORS_TOTAL];
owb_rmt_driver_info rmtDriverInfo;
OneWireBus *owb;
//Array for all temperatures (+1 beacuse of the analog solar meassure)
float temperatures[SENSORS_TOTAL + 1];
/*---------------------------------------------------------*/
/*              Variables for Pumps                        */
/*---------------------------------------------------------*/
int8_t statechache = 0;

void heatact(void *args){
    ESP_LOGI(TAG,"Toggle Solarpump");
    statechache ^= solarpumpe.mask;
    pumpsWrite(statechache);
/*---------------------------------------------------------*/
/*              Temperaturen Messen                        */
/*---------------------------------------------------------*/
    ESP_LOGI(TAG,"Messungen werden durchgeführt\n");
    char url[128] = "";
    ESP_ERROR_CHECK_WITHOUT_ABORT(tempReadAll(temperatures,url,dssensors));
    for(uint8_t a = 0; a < SENSORS_TOTAL + 1; a++){
        printf("Temperature %d is: %.2f\n",a,temperatures[a]);
    }
/*---------------------------------------------------------*/
/*              Temperaturen Senden                        */
/*---------------------------------------------------------*/
    ESP_LOGI(TAG,"Schicke alle Temperaturen zum Alpakagott");
    esp_err_t serverConnectionEstablished = httpGet((const char*)url);
    if(serverConnectionEstablished != ESP_OK){
        //Ab Hier gibt es nur den Automatischen betrieb
        ESP_LOGW(TAG,"Verbindung zum HTTP Server unmöglich");
    }
}

void app_main(){
    printf("\033[1;31m[E] Alpaka %d\n\033[0m",heizpumpe.gpio);
    ESP_LOGI(TAG,"Starte Pumpen");
    pumpsInit();
    ESP_LOGI(TAG, "Starte One-Wire Bus...");
    owb = tempInitBus(GPIO_ONE_WIRE, &rmtDriverInfo);
    ESP_LOGI(TAG, "One-Wire Bus wird konfiguriert...");
    tempDoSettings(owb);

    ESP_LOGI(TAG, "Starte Sensoren");
    //Init All Sensors
    ESP_ERROR_CHECK_WITHOUT_ABORT(tempBuildSensPtr(owb,dssensors));
    //Init Analog Temperature Meassurement
    tempAnalogInit();
    ESP_LOGI(TAG, "Starte WiFi");
    wifiInit();

    timerInit(&heatact);
    while(1){
        //NOP
    }    
}
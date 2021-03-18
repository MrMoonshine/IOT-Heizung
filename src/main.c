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
#include "heizung/solar.h"
#include "core/timer.h"

#define GPIO_ONE_WIRE TEMPERATURE_GPIO
#define BLINK_GPIO CHIPLED
#define MAX_DEVICES 11
#define DS18B20_ROM_CODE_LENGTH 17

#define I_HAVE_NO_CONNECTION_TO_YOUR_ROUTER "WiFi Interface is down. IP address is unassigned."

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


/**
 * @brief  Die Callback funktion vom Timer. Wird in einem Bestimmten Intevall immer wieder Aufgerufen.
 *
 * @param  args ungenutzt
*/
void heatact(void *args){
    //solarIsAutomatic();

    temperatures[TEMP_SOLAR] = 70;
    temperatures[TEMP_BUFFER] = 55;
    temperatures[TEMP_RUCKLAUF] = 50;
    temperatures[TEMP_VORLAUF] = 60;
    solarSetByTemp(temperatures);
/*---------------------------------------------------------*/
/*              Temperaturen Messen                        */
/*---------------------------------------------------------*/
    ESP_LOGI(TAG,"Messungen werden durchgeführt\n");
    char url[128] = "";
    //Fill the array with useless values
    memset(temperatures, -1000, sizeof(float)*(SENSORS_TOTAL + 1));
    /*BUS NOT INITIALIZED ERROR!*/
    ESP_ERROR_CHECK_WITHOUT_ABORT(tempReadAll(temperatures,url,dssensors));
    for(uint8_t a = 0; a < SENSORS_TOTAL + 1; a++){
        printf("Temperature %d is: %.2f\n",a,temperatures[a]);
    }
/*---------------------------------------------------------*/
/*              Temperaturen Senden                        */
/*---------------------------------------------------------*/
    ESP_LOGI(TAG,"Schicke alle Temperaturen zum Alpakagott");
    if(wifiStatus() == WIFI_IP_UP){
       esp_err_t serverConnectionEstablished = httpGet((const char*)url);
        if(serverConnectionEstablished != ESP_OK){
            //Ab Hier gibt es nur den Automatischen betrieb
            ESP_LOGW(TAG,"Verbindung zum HTTP Server unmöglich");
        } 
    }else{
        ESP_LOGW(TAG,I_HAVE_NO_CONNECTION_TO_YOUR_ROUTER);
    }    
}

void app_main(){
    printf("\033[1;31m[E] Alpaka %d\n\033[0m",heizpumpe.gpio);
    ESP_LOGI(TAG,"Starte Pumpen");
    pumpsInit();

    ESP_LOGI(TAG, "Starte WiFi");
    wifiInit();
 
/*---------------------------------------------------------*/
/*   Init all Temperature related things                   */
/*---------------------------------------------------------*/
    ESP_LOGI(TAG, "Starte One-Wire Bus...");
    owb = owb_rmt_initialize(
        &rmtDriverInfo,
        GPIO_ONE_WIRE,
        RMT_CHANNEL_1,
        RMT_CHANNEL_0
    ); 
    ESP_LOGI(TAG, "One-Wire Bus wird konfiguriert...");
    tempDoSettings(owb);
    ESP_LOGI(TAG, "Starte Sensoren");
    //Init All Sensors
    tempBuildSensPtr(owb,dssensors);
    //Init Analog Temperature Meassurement
    tempAnalogInit();
    timerInit(&heatact);
    while(1){
        //Used to avoid watchdog errors
        vTaskDelay(10 / portTICK_PERIOD_MS); 
    }    
}
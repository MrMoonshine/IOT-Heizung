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
#define TEMP_SAMPLES 5

#define I_HAVE_NO_CONNECTION_TO_YOUR_ROUTER "WiFi Interface is down. IP address is unassigned."

static const char* TAG = "Heizung";
/*---------------------------------------------------------*/
/*              Variables for One Wire Bus                 */
/*---------------------------------------------------------*/
DS18B20_Info dssensors[SENSORS_TOTAL];
owb_rmt_driver_info rmtDriverInfo;
OneWireBus *owb;
//Array for all temperatures (+1 beacuse of the analog solar meassure)
float temperatures[SENSORS_TOTAL + 1];
float tempchache[TEMP_SAMPLES][SENSORS_TOTAL + 1];
void fillTempChache(){
    for(uint8_t a = 0; a < TEMP_SAMPLES; a++){
        memset(tempchache[a], -1000, sizeof(float)*(SENSORS_TOTAL + 1));
    }
}
//Buffer for URL string
char tempurl[256] = "";
/*---------------------------------------------------------*/
/*              Variables for Pumps                        */
/*---------------------------------------------------------*/
static uint8_t blocker = 0;         //Anzahl der Aussetzer für die Solarpumpe

/**
 * @brief  Die Callback funktion vom Timer. Wird in einem Bestimmten Intevall immer wieder Aufgerufen.
 *
 * @param  args ungenutzt
*/
void heatact(void *args){
/*---------------------------------------------------------*/
/*              Temperaturen Messen                        */
/*---------------------------------------------------------*/
    ESP_LOGI(TAG,"Messungen werden durchgeführt\nPumpStates:%d",pumpsRead());
    //Fill the array with useless values
    memset(temperatures, -1000, sizeof(float)*(SENSORS_TOTAL + 1));
    ESP_ERROR_CHECK_WITHOUT_ABORT(tempReadArray(temperatures,dssensors));
/*---------------------------------------------------------*/
/*              Temperaturen Senden                        */
/*---------------------------------------------------------*/
    ESP_LOGI(TAG,"Schicke alle Temperaturen zum Alpakagott");
    if(
        wifiStatus() == WIFI_IP_UP &&
        tempArray2URL(temperatures,tempurl) == ESP_OK
    ){
        esp_err_t serverConnectionEstablished = httpGet((const char*)tempurl);
        if(serverConnectionEstablished != ESP_OK){
            //Ab Hier gibt es nur den Automatischen betrieb
            ESP_LOGW(TAG,"Verbindung zum HTTP Server unmöglich");
        } 
    }else{
        ESP_LOGW(TAG,I_HAVE_NO_CONNECTION_TO_YOUR_ROUTER);
    }
/*---------------------------------------------------------*/
/*              Pumpen                                     */
/*---------------------------------------------------------*/
    ESP_LOGI(TAG,"Requesting all Pumpstates");
    if(blocker == 0){
        //Set the Solarpump according to temperatures
        blocker = solarSetByTemp(temperatures);
    }else{
        blocker--;
    }
    
    if(solarIsAutomatic())
    pumpsWriteSolarIgnore(pumpsSync());
    else
    pumpsWrite(pumpsSync());   
    //pumpsWrite(pumpsSync());
    pumpsCache();
}

void app_main(){
    pumpsInit();
    ESP_LOGI(TAG,"Pumpen Gestartet");
    printf("\033[1;31m[E] Alpaka %d\n\033[0m",pumpsRead());

    ESP_LOGI(TAG, "Starte WiFi");
    wifiInit();
//Disable Mixer Relays
    gpio_pad_select_gpio(17);
    gpio_reset_pin(17); 
    gpio_set_direction(17, GPIO_MODE_OUTPUT);
    gpio_set_level(17,1);
    gpio_pad_select_gpio(5);
    gpio_reset_pin(5); 
    gpio_set_direction(5, GPIO_MODE_OUTPUT);
    gpio_set_level(5,1);
 
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
    fillTempChache();
    //Init Analog Temperature Meassurement
    tempAnalogInit();
    timerInit(&heatact);

    //Share Reset Reason
    httpResetInform();

    while(1){
        //Used to avoid watchdog errors
        vTaskDelay(10 / portTICK_PERIOD_MS); 
    }    
}
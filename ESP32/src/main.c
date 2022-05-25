#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"

#include "driver/gpio.h"
#include "sdkconfig.h"

#include "wifi.h"
#include <rest.h>
#include <string.h>
// Users for Rest API.
#include "/home/david/.secrets/heizung.h"
#ifndef HEIZUNG_USER
    #define HEIZUNG_USER "admin"
#endif
#ifndef HEIZUNG_PASSWORD
    #define HEIZUNG_USER "password"
#endif

#include "heizung/tempsensor.h"
#include "heizung/pumpen.h"
#include "heizung/romcodes.h"
#include "heizung/solar.h"
#include "core/timer.h"

static const char *HOSTNAME = "heizung";

#define GPIO_ONE_WIRE TEMPERATURE_GPIO
#define MAX_DEVICES 11
#define DS18B20_ROM_CODE_LENGTH 17

static const char* TAG = "Heizung";
/*---------------------------------------------------------*/
/*              Variables for Pumps                        */
/*---------------------------------------------------------*/
static uint8_t blocker = 0;         //Anzahl der Aussetzer für die Solarpumpe
/*---------------------------------------------------------*/
/*              Variables for One Wire Bus                 */
/*---------------------------------------------------------*/
owb_rmt_driver_info rmtDriverInfo;
OneWireBus *owb;
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
    ESP_ERROR_CHECK_WITHOUT_ABORT(temp_owb_read_sensors());
/*---------------------------------------------------------*/
/*              Pumpen                                     */
/*---------------------------------------------------------*/
    if(blocker == 0){
        heizung_temperatur_t *temps = temp_owb_list();
        float tsolar_vorlauf = TEMPERATURE_FAIL, tsolar_rucklauf = TEMPERATURE_FAIL, tbuffer_vorlauf = TEMPERATURE_FAIL;
        // Fetch temperatures from list
        while (temps)
        {
            switch (temps->id)
            {
            case TEMP_BUFFER:
                tbuffer_vorlauf = temps->value;
                break;
            case TEMP_RUCKLAUF:
                tsolar_rucklauf = temps->value;
                break;
            case TEMP_VORLAUF:
                tsolar_vorlauf = temps->value;
                break;
            default:
                break;
            }
            temps = temps->next;
        }
        // TEMPORARILY DISABLE. PUMP MUST BE SET MANUALLY
        //Set the Solarpump according to temperatures
        //blocker = solarSetByTemp(temp_analog_read(), tbuffer_vorlauf, tsolar_vorlauf, tsolar_rucklauf);
        blocker = 5;
        ESP_LOGI(TAG,"Solarpumpe evaluiert");
        // I am too lazy to create a new timer
        mdns_hostname_set(HOSTNAME);
        ESP_LOGI(TAG,"Bei der Gelegenheit auch gleich mDNS erneuert");
    }else{
        blocker--;
    }
    
    pumpsWrite(pumpsDefault());
}

void temperaturea_and_mdns(){
    heatact(NULL);
    // Renew A Record
    mdns_hostname_set(HOSTNAME);
}
/*
,---.    ,---.     .---.  _______ 
| .-.\   | .-'    ( .-._)|__   __|
| `-'/   | `-.   (_) \     )| |   
|   (    | .-'   _  \ \   (_) |   
| |\ \   |  `--.( `-'  )    | |   
|_| \)\  /( __.' `----'     `-'   
    (__)(__)                      
*/
/*
    @brief Linked List mit allen Benutzern
*/
extern rest_user_t *rest_api_users;
// Reset reason
static httpd_uri_t uri_reset = {
    .uri      = "/api/reset",
    .method   = HTTP_GET,
    .handler  = rest_default_reset_handler,
    .user_ctx = NULL
};
/*
    @brief Alle Temperaturen in einem API response. Permission: RO
*/

// URI
static httpd_uri_t uri_temperatures = {
    .uri      = "/api/temperatures",
    .method   = HTTP_GET,
    .handler  = heizung_api_temperatures,
    .user_ctx = NULL
};
// Pumps
static httpd_uri_t uri_pumps = {
    .uri      = "/api/pumps",
    .method   = HTTP_GET,
    .handler  = heizung_api_pumps,
    .user_ctx = NULL
};

static httpd_uri_t uri_pumps_post = {
    .uri      = "/api/pumps",
    .method   = HTTP_POST,
    .handler  = heizung_api_pumps,
    .user_ctx = NULL
};

void app_main(){
    // gpio setup
    pumpsInit();
    // Standard zustand
    pumpsWrite(pumpsDefault());
    ESP_LOGI(TAG,"Pumpen Gestartet");
    printf("\033[1;31m[E] Alpaka %d\n\033[0m",pumpsRead());

    wifiInit();
    ESP_LOGI(TAG, "WiFi gestartet");
    // Setup REST
    rest_api_users = NULL;
    rest_user_add(&rest_api_users, HEIZUNG_USER, HEIZUNG_PASSWORD, REST_USER_PERMISSION_RW);
    //Hostname
    rest_api_mdns(HOSTNAME);
    // Add resources
    rest_api_t *api = NULL;
    rest_api_add(&api, &uri_reset);
    rest_api_add(&api, &uri_temperatures);
    rest_api_add(&api, &uri_pumps);
    rest_api_add(&api, &uri_pumps_post);
    // Start server
    rest_api_start_server(api);
    ESP_LOGI(TAG, "REST API gestartet");
    // Delete API object. Keep user object!!!
    rest_api_delete(&api);
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
    ESP_LOGI(TAG, "Starte Sensoren...");
    //Init All Sensors
    ESP_ERROR_CHECK_WITHOUT_ABORT(temp_owb_add_all(owb));
    //Init Analog Temperature Meassurement
    temp_analog_init();
    // Every minute: read temperatures + renew mDNS record
    timerInit(TEMPSENSOR_READ_INTERVAL, &heatact);

    //Give the Wifi some time to initialize
    vTaskDelay(800 / portTICK_PERIOD_MS);
    //Fetch inistial states:
    heatact(NULL);
    while(1){
        //Used to avoid watchdog errors
        vTaskDelay(10 / portTICK_PERIOD_MS); 
    }    
}
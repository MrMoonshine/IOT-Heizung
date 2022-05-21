#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"

#include "driver/gpio.h"
#include "sdkconfig.h"

#include "wifi.h"
#include <rest.h>
#include <string.h>
// Users for Rest API
#include "/home/david/.secrets/heizung.h"

#include "heizung/tempsensor.h"
#include "heizung/pumpen.h"
#include "heizung/romcodes.h"
#include "heizung/solar.h"
#include "core/timer.h"

static const char *HOSTNAME = "heizung-test";

#define GPIO_ONE_WIRE TEMPERATURE_GPIO
#define BLINK_GPIO CHIPLED
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
DS18B20_Info dssensors[SENSORS_TOTAL];
owb_rmt_driver_info rmtDriverInfo;
OneWireBus *owb;
//Array for all temperatures (+1 beacuse of the analog solar meassure)
float temperatures[SENSORS_TOTAL + 1];
// Names
static const char *sensornames[SENSORS_TOTAL+1] = {
    "room",
    "red",
    "green",
    "blue",
    "white",
    "yellow",
    "brown",
    "solar"
};

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
/*              Pumpen                                     */
/*---------------------------------------------------------*/
    if(blocker == 0){
        //Set the Solarpump according to temperatures
        blocker = solarSetByTemp(temperatures);
        ESP_LOGI(TAG,"Solarpumpe evaluiert");
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
esp_err_t heizung_api_temperatures(httpd_req_t *req){
    /*
        Receives HTTP header data + does some Error handling
    */
    if(!rest_api_recv(req))
        return ESP_FAIL;
    /*
        A single function that handles authentication and error replies towards the client 
    */
    if(!rest_api_authenticate(req, rest_api_users, REST_USER_PERMISSION_RO))
        return ESP_FAIL;

    const char* json_tmp_temp_all = "{\"status\":%.1d, \"temperatures\":[%s]}";
    const char* json_tmp_temp_single = "{\"name\":\"%s\", \"value\":%.2f, \"unit\":\"°C\"}";
    // temp array element buffer
    size_t tarrebuff = strlen(json_tmp_temp_single) + 32; // 32 Geschätzt für Name, Wert und Komma (unsafe but ok)
    char* tarr = (char*)malloc((SENSORS_TOTAL + 1)*tarrebuff);
    if(!tarr){
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    strcpy(tarr, "");
    // Build JSON
    char* tarre = (char*)malloc(tarrebuff);
    if(!tarre){
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    for(uint8_t a = 0; a < SENSORS_TOTAL + 1; a++){
        strcpy(tarre, "");
        sprintf(tarre, json_tmp_temp_single, sensornames[a], temperatures[a]);
        // Copy to JSON array
        strcat(tarr, tarre);
        if(a != SENSORS_TOTAL)
            strcat(tarr, ",");
    }
    free(tarre);

    char* reply = (char*)malloc(strlen(json_tmp_temp_all) + tarrebuff*(SENSORS_TOTAL + 1));
    strcpy(reply, "");
    sprintf(reply, json_tmp_temp_all, 0, tarr);
    free(tarr);

    httpd_resp_set_type(req, "text/json");
    httpd_resp_send(req, reply, strlen(reply));

    free(reply);
    return ESP_OK;
}
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
    tempBuildSensPtr(owb,dssensors);
    //Init Analog Temperature Meassurement
    tempAnalogInit();
    // Every minute: read temperatures + renew mDNS record
    timerInit(TEMPSENSOR_READ_INTERVAL, &temperaturea_and_mdns);

    //Give the Wifi some time to initialize
    vTaskDelay(800 / portTICK_PERIOD_MS);
    //Fetch inistial states:
    heatact(NULL);
    while(1){
        //Used to avoid watchdog errors
        vTaskDelay(10 / portTICK_PERIOD_MS); 
    }    
}
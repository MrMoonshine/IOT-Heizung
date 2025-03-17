#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"

#include "driver/gpio.h"
#include "sdkconfig.h"

#include "wifi.h"
// Users for Rest API.
//#include "/home/david/.secrets/heizung.h"
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

#include <rest.h>
#include <string.h>

//static const char *HOSTNAME = "heizung";

#define GPIO_ONE_WIRE TEMPERATURE_GPIO
#define MAX_DEVICES 11
#define DS18B20_ROM_CODE_LENGTH 17

static const char* TAG = "Heizung";
/*---------------------------------------------------------*/
/*              Variables for Pumps                        */
/*---------------------------------------------------------*/
//Anzahl der Aussetzer für die Solarpumpe
volatile uint8_t blocker = 0;
// Zähler der HTTP Errors. Nach 5 fhler am stück kommt ein EMERGENCY SHUTDOWN
volatile uint8_t solarfailcount = 0;
static const uint8_t SOLAR_FAIL_COUNT_EMERGENCY = 5;
/*
    @brief ist die Solarpumpe im automatischen Betrieb?
*/
static bool solar_auto = true;
static bool solar_valid = false;
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
    temp_rest_init();
    // Read solar sensor. Is it valid?
    solar_valid = temp_rest_read() == ESP_OK;
    // Read OWB sensors
    ESP_ERROR_CHECK_WITHOUT_ABORT(temp_owb_read_sensors());
/*---------------------------------------------------------*/
/*              Pumpen                                     */
/*---------------------------------------------------------*/
    if(blocker == 0 && solar_auto){
        if(!solar_valid){
            solarfailcount++;
            if(solarfailcount >= SOLAR_FAIL_COUNT_EMERGENCY){
                // Turn off as Emergency!
                ESP_LOGE(TAG, "EMERGENCY STOP SOLARPUMP!!!");
                gpio_set_level(PUMP_SOLAR_GPIO,PUMP_OFF);
                return;
            }
        }else{
            solarfailcount = 0;
        }
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
        blocker = solarSetByTemp(temp_get_solar(), tbuffer_vorlauf, tsolar_vorlauf, tsolar_rucklauf);
        ESP_LOGI(TAG,"Solarpumpe evaluiert");
    }else{
        blocker--;
    }
    
    //pumpsWrite(pumpsDefault());
    //mdns_hostname_set(HOSTNAME);
}

esp_err_t heizung_api_solarauto(httpd_req_t *req){
    /*
        Receives HTTP header data + does some Error handling
    */
    if(!rest_api_recv(req))
        return ESP_FAIL;

    const char* solarautokey = "manuell";
    // Allocade length accordingly
    size_t queryLen = httpd_req_get_url_query_len(req) + 1;
    if(queryLen > 1){
        char* query;
        query = (char*)malloc(queryLen);
        if(!query){
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }
        // Safe copy query string
        httpd_req_get_url_query_str(req, query, queryLen);

        const size_t rvbuffLen = 24;
        char* rvbuff = (char*)malloc(rvbuffLen);
        if(rvbuff == NULL){
            free(query);
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }
        memset(rvbuff, ' ', rvbuffLen);
        if(ESP_OK == httpd_query_key_value(query, solarautokey, rvbuff, rvbuffLen)){
            int manually = atoi(rvbuff);
            // Set automatic mode. Invert manual
            solar_auto = manually < 1;
        }
        free(rvbuff);
        free(query);
        query = NULL;
    }
    
    const char* json_tmp = "{\"status\":%.1d, \"modus\":\"%s\",\"solar_valid\":\"%s\"}";
    size_t rlen = strlen(json_tmp) + 16 + 10;
    // Send back buffer
    char* reply = (char*)malloc(rlen);
    strcpy(reply, "");
    sprintf(
        reply,
        json_tmp,
        0,
        solar_auto ? "automatisch" : solarautokey,
        solar_valid ? "OK" : "EMERGENCY"
    );

    httpd_resp_set_type(req, "text/json");
    httpd_resp_send(req, reply, strlen(reply));

    free(reply);
    return ESP_OK;
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

static httpd_uri_t uri_pumps_solar = {
    .uri      = "/api/solar",
    .method   = HTTP_GET,
    .handler  = heizung_api_solarauto,
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
    //rest_user_t* rest_api_users = rest_user_list();
    //rest_user_add(&rest_api_users, HEIZUNG_USER, HEIZUNG_PASSWORD, REST_USER_PERMISSION_RW);
    //Hostname
    //rest_api_mdns(HOSTNAME);
    // Add resources
    rest_api_t *api = NULL;
    rest_api_add(&api, &uri_reset);
    rest_api_add(&api, &uri_temperatures);
    rest_api_add(&api, &uri_pumps);
    rest_api_add(&api, &uri_pumps_solar);
    // Start server
    rest_api_start_server(api);
    ESP_LOGI(TAG, "REST API gestartet");
    // Delete API object. Keep user object!!!
    rest_api_delete(&api);
    //Disable Mixer Relays
    //gpio_pad_select_gpio(17);
    gpio_reset_pin(17); 
    gpio_set_direction(17, GPIO_MODE_OUTPUT);
    gpio_set_level(17,1);
    //gpio_pad_select_gpio(5);
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
    //Init REST Temperature Meassurement
    //temp_rest_init();

    //Give the Wifi some time to initialize
    vTaskDelay(4000 / portTICK_PERIOD_MS);
    
    while(1){
        heatact(NULL);
        //Wait a minute
        vTaskDelay(TEMPSENSOR_READ_INTERVAL * 1000 / portTICK_PERIOD_MS);
    }    
}
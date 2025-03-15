#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"

#include "driver/gpio.h"
#include "sdkconfig.h"

#include <include/owb.h>
#include <include/owb_rmt.h>
#include <include/ds18b20.h>

#include "wifi.h"

#include <rest.h>
#include <string.h>

#define DS18B20_RESOLUTION   (DS18B20_RESOLUTION_12_BIT)
#define TEMPERATURE_FAIL (-300.0f)

static const char* TAG = "Main";

static const gpio_num_t LED_ONBOARD = 2; // Onboard LED
static const gpio_num_t GPIO_ONE_WIRE = 15; // GPIO15
/*
    Test Setup:
    5c 00000bc75539 28 DS18B20
    15 0c0c031d9b92 3b MAX31850
*/
/*static const OneWireBus_ROMCode ROMCODE_DALLAS = {
    .fields.family = {0x28},
    .fields.serial_number = {0x39, 0x55, 0xc7, 0x0b, 0x00, 0x00},
    .fields.crc = {0x5c}
};*/

static const OneWireBus_ROMCode ROMCODE_MAXIM = {
    .fields.family = {0x3b},
    .fields.serial_number = {0x92, 0x9b, 0x1d, 0x03, 0x0c, 0x0c},
    .fields.crc = {0x15}
};

static DS18B20_Info sensor_max;

void universal_read(DS18B20_Info *info, float* temp, float* cj){
    *temp = TEMPERATURE_FAIL;
    if(!info->bus /*|| !&info->rom_code*/){
        ESP_LOGW(TAG,"DS info ist NULL");
        return;
    }

    ds18b20_convert_all(info->bus);
    ds18b20_wait_for_conversion(info);
    // If a io var for CJ has been specified do thermocouple
    if(cj){
        MAX31850_ERROR err = max31850_read_temp(info,temp, cj);
        if(err != MAX31850_OK){
            ESP_LOGE(TAG,"MAX31850 Error %.2x", err);
            *temp = TEMPERATURE_FAIL;
            *cj = TEMPERATURE_FAIL;
        }
        return;
    }

    if(ds18b20_read_temp(info,temp) == DS18B20_ERROR_DEVICE){
        ESP_LOGE(TAG,"DS18B20 Device Error");
        *temp = TEMPERATURE_FAIL;
    }
}


esp_err_t rest_read(httpd_req_t *req, const char* pattern){
    // Flash blue onboard LED on API Call
    gpio_set_level(LED_ONBOARD, 1);

    float tc = -2048.0f;
    float cj = -2048.0f;
    universal_read(&sensor_max, &tc, &cj);

    int sizetc = snprintf(NULL, 0, "%.2f", tc);
    int sizecj = snprintf(NULL, 0, "%.2f", cj);
    
    size_t buffSize = strlen(pattern) + sizetc + sizecj + 1;    // +1 for \0
    char* buff = (char*)malloc(buffSize);
    if(buff == NULL){
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    sprintf(buff, pattern, tc, cj);
    // We're dealing with json probabbly
    if(buff[0] == '{')
        httpd_resp_set_type(req, "text/json");

    httpd_resp_send(req, buff, strlen(buff));
    free(buff);
    // Set LED back to off
    gpio_set_level(LED_ONBOARD, 0);
    return ESP_OK;
}

esp_err_t rest_read_raw(httpd_req_t *req){
    return rest_read(req, "%.2f\n%.2f");
}

esp_err_t rest_read_json(httpd_req_t *req){
    return rest_read(req, "{\"thermocouple\":%.2f, \"cold_junction\":%.2f}");
}

httpd_uri_t uri_read_raw = {
    .uri      = "/raw",
    .method   = HTTP_GET,
    .handler  = rest_read_raw,
    .user_ctx = NULL
};

httpd_uri_t uri_read_json = {
    .uri      = "/json",
    .method   = HTTP_GET,
    .handler  = rest_read_json,
    .user_ctx = NULL
};

void app_main(){
    // handle WiFi
    wifiInit();

    rest_api_t *api = NULL;
    rest_api_add(&api, &uri_read_raw);
    rest_api_add(&api, &uri_read_json);

    // OWB stuff
    owb_rmt_driver_info rmtDriverInfo;
    OneWireBus *owb;

    uint8_t counter = 0;
    gpio_reset_pin(LED_ONBOARD); 
    gpio_set_direction(LED_ONBOARD, GPIO_MODE_INPUT_OUTPUT);

    ESP_LOGI(TAG, "Starte One-Wire Bus...");
    owb = owb_rmt_initialize(
        &rmtDriverInfo,
        GPIO_ONE_WIRE,
        RMT_CHANNEL_1,
        RMT_CHANNEL_0
    );
    owb_use_crc(owb, true);  // enable CRC check for ROM code

    char romstr1[17];
    owb_string_from_rom_code(ROMCODE_MAXIM, romstr1, sizeof(romstr1));
    printf("Init Sensor with Code %s\n", romstr1);
    ds18b20_init(&sensor_max, owb, ROMCODE_MAXIM);
    ds18b20_use_crc(&sensor_max, true);           // enable CRC check on all reads

    // Start API server
    rest_api_start_server(api);
    // Delete API config object.
    rest_api_delete(&api);

    while(1){
        //Wait a bit
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
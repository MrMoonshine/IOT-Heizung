#include "wifi.h"

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static const char *PROG_NAME = "WiFi-Handler";
static EventGroupHandle_t sWifiEventGroup;
//Setup NVS
static void initNVS(){
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(PROG_NAME,"Erasing NVS-flash due to failure in initialisation");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );
}

//This Function handles the DHCP procedure
static void eventHandler(
    void *ctx,
    esp_event_base_t *eventBase,
    int32_t eventId,
    void* eventData
){
    //if wifi should start
    if(eventBase == WIFI_EVENT){
        switch(eventId){
            case WIFI_EVENT_STA_START: esp_wifi_connect(); break;
            case WIFI_EVENT_STA_DISCONNECTED: esp_wifi_connect(); break;
            default: break;
        }
    }

    if(eventBase == IP_EVENT){
        if(eventId == IP_EVENT_STA_GOT_IP){
            ip_event_got_ip_t* event = (ip_event_got_ip_t*) eventData;
            ESP_LOGI(PROG_NAME, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
            xEventGroupSetBits(sWifiEventGroup, WIFI_CONNECTED_BIT);
        }
    }
}

esp_err_t wifiInit(){
    initNVS();
    sWifiEventGroup = xEventGroupCreate();

    ESP_LOGI(PROG_NAME,"Setting up WiFi");
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instanceAnyId, instanceGotIP;
    ESP_ERROR_CHECK(
        esp_event_handler_instance_register(
            WIFI_EVENT,
            ESP_EVENT_ANY_ID,
            &eventHandler,
            NULL,
            &instanceAnyId
        )
    );
    ESP_ERROR_CHECK(
        esp_event_handler_instance_register(
            IP_EVENT,
            IP_EVENT_STA_GOT_IP,
            &eventHandler,
            NULL,
            &instanceGotIP
        )
    );
    wifi_config_t wifiConfig = {
        .sta = {
            .ssid = WAP_HOME_SSID,
            .password = WAP_HOME_PASSWORD,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        }
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifiConfig));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(PROG_NAME,"Wifi statred successfully!");

    return ESP_OK;
}
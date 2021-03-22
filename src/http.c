#include "wifi.h"

static const char* HTTPTAG = "HTTP";
//#define HTTP_SMALL_BUFF_SIZE 64
#define HTTP_PROTOCOL_REQ_TAG "http://"

static esp_err_t _http_event_handle(esp_http_client_event_t *evt)
{   
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(HTTPTAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(HTTPTAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(HTTPTAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(HTTPTAG, "HTTP_EVENT_ON_HEADER");
            printf("%.*s", evt->data_len, (char*)evt->data);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(HTTPTAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            if (!esp_http_client_is_chunked_response(evt->client)) {
                printf("%.*s\n", evt->data_len, (char*)evt->data);
            }
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(HTTPTAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGD(HTTPTAG, "HTTP_EVENT_DISCONNECTED");
            break;
    }
    return ESP_OK;
}

esp_err_t httpGet(const char* url){
        //Handle the majority of errors
    if(strlen(url) < 1 ||
        strncmp(url, HTTP_PROTOCOL_REQ_TAG, strlen(HTTP_PROTOCOL_REQ_TAG)) != 0
    ){
        ESP_LOGE(HTTPTAG,"Invalid URL!");
        return ESP_FAIL;
    }else if(wifiStatus() != WIFI_IP_UP){
        ESP_LOGW(HTTPTAG,"WiFi Connection Lost! No GET will be performed!");
        return ESP_FAIL;
    }
    esp_http_client_config_t config = {
        .url = url,
        .event_handler = _http_event_handle,
        .method = HTTP_METHOD_GET
    };
    
    esp_http_client_handle_t client = esp_http_client_init(&config);

    //ESP_LOGD(HTTPTAG,"Request to %s",config.url);
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        ESP_LOGD(HTTPTAG, "Status = %d, content_length = %d",
        esp_http_client_get_status_code(client),
        esp_http_client_get_content_length(client));
    }

    esp_http_client_cleanup(client);
    return err;
}

esp_err_t   httpResetInform(){
    char rsturl[HTTP_URL_BUFF_SIZE] = "";
    esp_reset_reason_t reason = esp_reset_reason();
    uint8_t rstid = 0;
    switch (reason){
        case ESP_RST_UNKNOWN:   rstid = DB_RST_REASON_UNKNOWN;    break;
        case ESP_RST_POWERON:   rstid = DB_RST_REASON_POWERON;    break;
        case ESP_RST_EXT:       rstid = DB_RST_REASON_EXT;        break;
        case ESP_RST_SW:        rstid = DB_RST_REASON_SW;         break;
        case ESP_RST_PANIC:     rstid = DB_RST_REASON_PANIC;      break;
        case ESP_RST_INT_WDT:   rstid = DB_RST_REASON_INT_WDT;    break;
        case ESP_RST_TASK_WDT:  rstid = DB_RST_REASON_TASK_WDT;   break;
        case ESP_RST_WDT:       rstid = DB_RST_REASON_WDT;        break;
        case ESP_RST_DEEPSLEEP: rstid = DB_RST_REASON_DEEPSLEEP;  break;
        case ESP_RST_BROWNOUT:  rstid = DB_RST_REASON_BROWNOUT;   break;
        case ESP_RST_SDIO:      rstid = DB_RST_REASON_SDIO;       break;
        default:                return ESP_FAIL;                  break;
    }
    sprintf(rsturl,"%s?reset=%d",URL_LOG,rstid);
    ESP_LOGI(HTTPTAG,"Sending Reset-Information to %s",rsturl);
    return httpGet(rsturl);
}
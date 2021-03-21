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
    char rsturl[HTTP_URL_BUFF_SIZE] = "http://queen:8000/esp/reset/";
    esp_reset_reason_t reason = esp_reset_reason();
    switch (reason){
    case ESP_RST_UNKNOWN:
        strcat(rsturl,"UNKNOWN");
        break;
    case ESP_RST_POWERON:
        strcat(rsturl,"POWERON");
        break;
    case ESP_RST_EXT:
        strcat(rsturl,"EXT");
        break;
    case ESP_RST_SW:
        strcat(rsturl,"SW");
        break;
    case ESP_RST_PANIC:
        strcat(rsturl,"PANIC");
        break;
    case ESP_RST_INT_WDT:
        strcat(rsturl,"INT_WDT");
        break;
    case ESP_RST_TASK_WDT:
        strcat(rsturl,"TASK_WDT");
        break;
    case ESP_RST_WDT:
        strcat(rsturl,"WDT");
        break;
    case ESP_RST_DEEPSLEEP:
        strcat(rsturl,"DEEPSLEEP");
        break;
    case ESP_RST_BROWNOUT:
        strcat(rsturl,"BROWNOUT");
        break;
    case ESP_RST_SDIO:
        strcat(rsturl,"SDIO");
        break;
    default:
        return ESP_FAIL;
        break;
    }
    ESP_LOGI(HTTPTAG,"Sending Reset-Information to %s",rsturl);
    return httpGet(rsturl);
}
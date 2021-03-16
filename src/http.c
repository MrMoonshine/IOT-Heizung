#include "wifi.h"

static const char* HTTPTAG = "HTTP";
#define HTTP_SMALL_BUFF_SIZE 64
static char smallHttpBuff[HTTP_SMALL_BUFF_SIZE] = "";

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
                printf("%.*s", evt->data_len, (char*)evt->data);
                if(evt->data_len < HTTP_SMALL_BUFF_SIZE)
                strcpy(smallHttpBuff,(char*)evt->data);
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
    if(strlen(url) < 1){
        ESP_LOGE(HTTPTAG,"Invalid URL!");
        return ESP_FAIL;
    }
    esp_http_client_config_t config = {
        .url = url,
        .event_handler = _http_event_handle,
        .port = 80
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

esp_err_t httpGetBuffer(const char* url,char* buffer, size_t buffersize){
    if(strlen(url) < 1){
        ESP_LOGE(HTTPTAG,"Invalid URL!");
        return ESP_FAIL;
    }
    esp_http_client_config_t config = {
        .url = url,
        .event_handler = _http_event_handle,
        .port = 80
    };
    
    esp_http_client_handle_t client = esp_http_client_init(&config);

    //ESP_LOGD(HTTPTAG,"Request to %s",config.url);
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        ESP_LOGD(HTTPTAG, "Status = %d, content_length = %d",
        esp_http_client_get_status_code(client),
        esp_http_client_get_content_length(client));

        if(HTTP_SMALL_BUFF_SIZE >= buffersize)
        strcpy(buffer,smallHttpBuff);
    }

    esp_http_client_cleanup(client);
    return err;
}
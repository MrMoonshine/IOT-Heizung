#include "solar.h"
static const char* SOLARTAG = "Solar";
uint8_t solarSetByTemp(float *temps){
    //Anzahl der Aussetzer
    uint8_t blocker = 0;
    //Exit on wrong Meassurements
    if(
        temps[TEMP_SOLAR]       < ZERO_KELVIN ||    //Solar is Analog. No error occured here, but it's still wrong.
        temps[TEMP_BUFFER]      < ZERO_KELVIN ||
        temps[TEMP_RUCKLAUF]    < ZERO_KELVIN ||
        temps[TEMP_VORLAUF]     < ZERO_KELVIN
    ){
        ESP_LOGW(SOLARTAG,"Ungültige Messwerte! Solarpumpe bleibt im gleichen Zustand");
        return blocker;
    }

    //Es ist Invertiert weil die Relays der Pumpen invertiert sind
    if(gpio_get_level(solarpumpe.gpio)){
        //Solarpump is not running at this point
        if(temps[TEMP_SOLAR] > temps[TEMP_BUFFER] + SOLAR_TO_BUFF_OFFSET){
            ESP_LOGI(SOLARTAG,"Solarpumpe einschalten.");
            gpio_set_level(solarpumpe.gpio,PUMP_ON);
            blocker = SOLAR_BLOCK_CYCLE_AFTER_ENABLE;
        }
    }else{
        //Solarpump is running
        if(
            temps[TEMP_VORLAUF] < temps[TEMP_RUCKLAUF] ||     //Vorlauf ist kälter als Rücklauf
            temps[TEMP_SOLAR] < temps[TEMP_BUFFER]          //oder Solarpannel ist kälter als Buffer
        ){  
            ESP_LOGI(SOLARTAG,"Solarpumpe ausschalten.");
            gpio_set_level(solarpumpe.gpio,PUMP_OFF);
            blocker = 0;
        }
    }
    return blocker;
}

static int8_t srvsolatstate;
static esp_err_t _http_solar_auto_event_handle(esp_http_client_event_t *evt)
{   
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(SOLARTAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(SOLARTAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(SOLARTAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(SOLARTAG, "HTTP_EVENT_ON_HEADER");
            printf("%.*s", evt->data_len, (char*)evt->data);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(SOLARTAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            if (!esp_http_client_is_chunked_response(evt->client)) {
                ESP_LOGD(SOLARTAG,"Solarpumpe wird %.*s gesteuert.\n", evt->data_len, (char*)evt->data);
                if(strstr((char*)evt->data,"automatic") != NULL)
                srvsolatstate = 1;
                else if(strstr((char*)evt->data,"manual") != NULL)
                srvsolatstate = 0;
                else
                srvsolatstate = -1;
            }
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(SOLARTAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGD(SOLARTAG, "HTTP_EVENT_DISCONNECTED");
            break;
    }
    return ESP_OK;
}

bool solarIsAutomatic(){
    srvsolatstate = -1;
    esp_http_client_config_t config = {
        .url = SOLAR_HTTP_AUTO_MANUAL_URL,
        .event_handler = _http_solar_auto_event_handle,
        .port = 80
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_perform(client);
    esp_http_client_cleanup(client);
    return srvsolatstate == 0 ? false : true;
}
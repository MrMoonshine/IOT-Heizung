#include "pumpen.h"
static const char* PUMPTAG = "Pumpensteuerung";
//There are 6 Pumps in total
static const Pumpe* allpumps[PUMPENANZAHL] = {
    &heizpumpe,
    &solarpumpe,
    &redundancy1,
    &bufferpumpe,
    &zwischenpumpe,
    &warmepumpe,
};

void initPump(const Pumpe *pump){
    gpio_pad_select_gpio(pump->gpio);
    gpio_reset_pin(pump->gpio); 
    gpio_set_direction(pump->gpio, GPIO_MODE_INPUT_OUTPUT);
}

esp_err_t pumpsInit(){
    int8_t states = 0xFF;
    if(recoveryStartup(&states) != ESP_OK)
    ESP_LOGW(PUMPTAG,"Zustände der Pumpen konnten nicht wieder hergestellt werden! Jetzt sind alle aus");

    for(uint8_t a = 0; a < PUMPENANZAHL; a++)
    initPump(allpumps[a]);

    ESP_LOGI(PUMPTAG,"Initial States: %d",states);
    pumpsWrite(states);
    return ESP_OK;
}

esp_err_t pumpsWrite(int8_t states){
    if(states < 0)
    return ESP_FAIL;
    //printf("State is %d\nMask is %d\n",states,allpumps[1]->mask);
    for(uint8_t a = 0; a < PUMPENANZAHL; a++){
        //Die Logik Level für die Pumpen sind Invertiert
        gpio_set_level(
            allpumps[a]->gpio,
            allpumps[a]->mask & states ? PUMP_OFF : PUMP_ON
        );
    }
    return ESP_OK;
}

esp_err_t pumpsWriteSolarIgnore(int8_t states){
    int8_t sistates = states;
    if(gpio_get_level(solarpumpe.gpio)){
        sistates |= solarpumpe.mask;
    }else{
        sistates &= ~solarpumpe.mask;
    }
    return pumpsWrite(sistates);
}

static int8_t srvpumpstate;
//This event will set the srvpumpstate to an apropriate state
static esp_err_t _http_pump_event(esp_http_client_event_t *evt)
{   
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(PUMPTAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(PUMPTAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(PUMPTAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(PUMPTAG, "HTTP_EVENT_ON_HEADER");
            printf("%.*s", evt->data_len, (char*)evt->data);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(PUMPTAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            if (!esp_http_client_is_chunked_response(evt->client)) {
                ESP_LOGD(PUMPTAG,"%.*s\n", evt->data_len, (char*)evt->data);
                if(strstr((char*)evt->data,"ON") != NULL)
                srvpumpstate = PUMP_ON;
                else if(strstr((char*)evt->data,"OFF") != NULL)
                srvpumpstate = PUMP_OFF;
            }
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(PUMPTAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGD(PUMPTAG, "HTTP_EVENT_DISCONNECTED");
            break;
    }
    return ESP_OK;
}

//hilfsfunktion für pumps sync
#define PUMPREQ2MASK_URL_BUFFER_SIZE 128
static char urlbuff[PUMPREQ2MASK_URL_BUFFER_SIZE];
int8_t pumpReq2Stt(gpio_num_t pump){
    srvpumpstate = PUMP_ERR;

    sprintf(urlbuff,"%s?pump=%d",URL_PUMPS,pump);
    ESP_LOGD(PUMPTAG,"Requesting Pumpsate of %d. (GPIO)\nURL is: %s",pump,urlbuff);

    esp_http_client_config_t config = {
        .url = (const char*)urlbuff,
        .event_handler = _http_pump_event,
        .port = 80
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_perform(client);
    esp_http_client_cleanup(client);

    //ESP_LOGI(PUMPTAG,"State is: %d",srvpumpstate);
    return srvpumpstate;
}

int8_t pumpsSync(){
    int8_t states = 0;
    for(uint8_t a = 0; a < PUMPENANZAHL; a++){
        int8_t mskt = pumpReq2Stt(allpumps[a]->gpio);
        if(mskt < 0)
        return mskt;
        states |= mskt * allpumps[a]->mask;
    }
    return states;
}

int8_t pumpsRead(){
    int8_t states = 0;
    for(uint8_t a = 0; a < PUMPENANZAHL; a++){
        states |= gpio_get_level(allpumps[a]->gpio) ? 0 : allpumps[a]->mask;
    }
    return states;
}

esp_err_t pumpsCache(){
    int8_t states = 0, curstates = pumpsRead();
    if(recoveryStartup(&states) != ESP_OK){
       ESP_LOGW(PUMPTAG,"Zustände konnten nicht abgerufen werden!");
       return ESP_FAIL; 
    }

    //ESP_LOGI(PUMPTAG,"State compare: %d\t%d",states,curstates);
    if(states == curstates)
    return ESP_OK;
    
    recoveryWrite(curstates);
    ESP_LOGI(PUMPTAG,"States sind im NVS geschrieben: %d",curstates);
    return ESP_OK;
}
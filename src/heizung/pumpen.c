#include "pumpen.h"
void initPump(const Pumpe *pump){
    gpio_pad_select_gpio(pump->gpio);
    gpio_reset_pin(pump->gpio); 
    gpio_set_direction(pump->gpio, GPIO_MODE_INPUT_OUTPUT);
}

esp_err_t pumpsInit(){
    int8_t states = 0;
    if(recoveryStartup(&states) != ESP_OK)
    ESP_LOGW(PUMPTAG,"Zustände der Pumpen konnten nicht wieder hergestellt werden! Jetzt sind alle aus");

    for(uint8_t a = 0; a < PUMPENANZAHL; a++)
    initPump(allpumps[a]);

    pumpsWrite(states);
    return ESP_OK;
}

esp_err_t pumpsWrite(int8_t states){
    //printf("State is %d\nMask is %d\n",states,allpumps[1]->mask);
    for(uint8_t a = 0; a < PUMPENANZAHL; a++){
        //Die Logik Level für die Pumpen sind Invertiert
        gpio_set_level(
            allpumps[a]->gpio,
            allpumps[a]->mask & states ? PUMP_ON : PUMP_OFF
        );
    }
    return ESP_OK;
}

//hilfsfunktion für pumps sync
#define PUMPREQ2MASK_RECV_BUFFER_SIZE 64
#define PUMPREQ2MASK_URL_BUFFER_SIZE 64
static char urlbuff[PUMPREQ2MASK_URL_BUFFER_SIZE];
static char recvbuff[PUMPREQ2MASK_RECV_BUFFER_SIZE];
int8_t pumpReq2Stt(gpio_num_t pump){
    strcpy(urlbuff,"");
    strcpy(recvbuff,"");

    sprintf(urlbuff,"%s?pump=%d",PUMP_SET_URL,pump);
    

    if(httpGetBuffer(urlbuff,recvbuff,PUMPREQ2MASK_RECV_BUFFER_SIZE) != ESP_OK)
    return -1;

    if(strstr(recvbuff,"ON") != NULL){
        return 1; 
    } else if(strstr(recvbuff,"OFF") != NULL){
        return 0;
    }

    return -1;
}

int8_t pumpsSync(bool solarauto){
    int8_t states = 0;
    for(uint8_t a = 0; a < PUMPENANZAHL; a++){
        if(allpumps[a]->gpio == solarpumpe.gpio && solarauto){
            //Pumpe ist aus => 0; Pumpe ist ein => | mask
            states |= gpio_get_level(solarpumpe.gpio) ? 0 : solarpumpe.mask;
        }else{
            int8_t mskt = pumpReq2Stt(allpumps[a]->gpio);
            if(mskt < 0)
            return mskt;
            states |= mskt * allpumps[a]->mask;
        }
    }
    return states;
}
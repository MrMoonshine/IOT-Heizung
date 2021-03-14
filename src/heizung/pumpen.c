#include "pumpen.h"
void initPump(const Pumpe *pump){
   gpio_reset_pin(pump->gpio); 
   gpio_set_direction(pump->gpio, GPIO_MODE_OUTPUT);
}

esp_err_t pumpsInit(){
    int8_t states = 0;
    if(recoveryStartup(&states) != ESP_OK)
    ESP_LOGW(PUMPTAG,"Zustände der Pumpen konnten nicht wieder hergestellt werden! Jetzt sind alle aus");

    for(uint8_t a = 0; a < PUMPENANZAHL; a++)
    pumpsInit(allpumps+a);

    pumpsWrite(states);
    return ESP_OK;
}

esp_err_t pumpsWrite(int8_t states){
    for(uint8_t a = 0; a < PUMPENANZAHL; a++){
        //Die Logik Level für die Pumpen sind Invertiert
        gpio_set_level(
            allpumps[a]->gpio,
            allpumps[a]->mask & states ? 0 : 1
        );
    }
    return ESP_OK;
}
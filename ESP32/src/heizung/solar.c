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
    if(gpio_get_level(PUMP_SOLAR_GPIO)){
        //Solarpump is not running at this point
        if(temps[TEMP_SOLAR] > temps[TEMP_BUFFER] + SOLAR_TO_BUFF_OFFSET){
            ESP_LOGI(SOLARTAG,"Solarpumpe einschalten.");
            gpio_set_level(PUMP_SOLAR_GPIO,PUMP_ON);
            blocker = SOLAR_BLOCK_CYCLE_AFTER_ENABLE;
        }
    }else{
        //Solarpump is running
        if(
            temps[TEMP_VORLAUF] < temps[TEMP_RUCKLAUF] ||     //Vorlauf ist kälter als Rücklauf
            temps[TEMP_SOLAR] < temps[TEMP_BUFFER]          //oder Solarpannel ist kälter als Buffer
        ){  
            ESP_LOGI(SOLARTAG,"Solarpumpe ausschalten.");
            gpio_set_level(PUMP_SOLAR_GPIO,PUMP_OFF);
            blocker = 0;
        }
    }
    return blocker;
}
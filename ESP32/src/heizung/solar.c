#include "solar.h"
static const char* SOLARTAG = "Solar";
uint8_t solarSetByTemp(float solar, float buffer, float vorlauf, float rucklauf){
    //Anzahl der Aussetzer
    uint8_t blocker = 0;
    //Exit on wrong Meassurements
    if(
        solar       < ZERO_KELVIN ||    //Solar is Analog. No error occured here, but it's still wrong.
        buffer      < ZERO_KELVIN ||
        rucklauf    < ZERO_KELVIN ||
        vorlauf     < ZERO_KELVIN
    ){
        ESP_LOGW(SOLARTAG,"Ungültige Messwerte! Solarpumpe bleibt im gleichen Zustand");
        return blocker;
    }

    //Es ist Invertiert weil die Relays der Pumpen invertiert sind
    if(gpio_get_level(PUMP_SOLAR_GPIO)){
        //Solarpump is not running at this point
        if(
            solar > buffer + SOLAR_TO_BUFF_OFFSET &&
            solar >= SOLAR_ENABLE_MIN_TEMP // Solarpanel muss min 40°C haben, weil die B-Formel exponentiell ist. Bei der anpassung in Matlab kann man sehen, dass die Steigung zu gering ist um temperaturen < 40°C damit zu messen.
        ){
            ESP_LOGI(SOLARTAG,"Solarpumpe einschalten.");
            gpio_set_level(PUMP_SOLAR_GPIO,PUMP_ON);
            blocker = SOLAR_BLOCK_CYCLE_AFTER_ENABLE;
        }
    }else{
        //Solarpump is running
        if(
            vorlauf < rucklauf ||     //Vorlauf ist kälter als Rücklauf
            solar < buffer          //oder Solarpannel ist kälter als Buffer
        ){  
            ESP_LOGI(SOLARTAG,"Solarpumpe ausschalten.");
            gpio_set_level(PUMP_SOLAR_GPIO,PUMP_OFF);
            blocker = 0;
        }
    }
    return blocker;
}
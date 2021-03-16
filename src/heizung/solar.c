#include "solar.h"
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

    //Inverted!
    if(gpio_get_level(solarpumpe.gpio)){
        //Solarpump is not running at this point
        if(temps[TEMP_SOLAR] > temps[TEMP_BUFFER] + SOLAR_TO_BUFF_OFFSET){
            ESP_LOGI(SOLARTAG,"Solarpumpe einschalten.");
            gpio_set_level(solarpumpe.gpio,PUMP_ON);
            blocker = 3;
        }
    }else{
        //Solarpump is running
        if(
            temps[TEMP_VORLAUF] < temps[TEMP_RUCKLAUF] ||     //Vorlauf ist kälter als Rücklauf
            temps[TEMP_SOLAR] < temps[TEMP_BUFFER]          //oder Solarpannel ist kälter als Buffer
        ){  
            ESP_LOGI(SOLARTAG,"Solarpumpe ausschalten.");
            gpio_set_level(solarpumpe.gpio,PUMP_OFF);
            blocker = 5;
        }
    }
    return blocker;
}

bool solarIsAutomatic(){
    char buffer[SOLAR_HTTP_BUFFER_SIZE] = "";
    httpGetBuffer(
        SOLAR_HTTP_AUTO_MANUAL_URL,
        buffer,
        SOLAR_HTTP_BUFFER_SIZE
    );

    if(strlen(buffer) < 1)
    return false;

    ESP_LOGI(SOLARTAG,"Preferred solarmode is: %s",buffer);
    if(strstr(buffer,"automatic") != NULL)
    return true;
    else
    return false;
}
#include "tempsensor.h"

static const char* OWBTAG = "OneWireBus";

bool tempVerify(OneWireBus *bus,OneWireBus_ROMCode *dev){
    bool isPresent = false;
    
    owb_status searchStatus = owb_verify_rom(
        bus,
        *dev,
        &isPresent
    );

    if(searchStatus != OWB_STATUS_OK){
        ESP_LOGE(OWBTAG,"Failed to search device");
        return false;
    }
    
    if(!isPresent){
        char *devCode;
        devCode = (char*)malloc(OWB_ROM_CODE_STRING_LENGTH);
        if(devCode == NULL)
        return false;
        owb_string_from_rom_code(*dev,devCode,sizeof(devCode));
        ESP_LOGW(OWBTAG,"Failed to find sensor: %s",devCode);

        free(devCode);
        return false;
    }
    return true;
}

void tempDoSettings(OneWireBus *owb){
    //Enable CRC
    owb_use_crc(owb, true);

    //Check for parasitric devices
    bool parasitic_power = false;
    ds18b20_check_for_parasite_power(owb, &parasitic_power);
    if (parasitic_power) {
        printf("Parasitic-powered devices detected\n");
    }

    owb_use_parasitic_power(owb, parasitic_power);
#ifdef CONFIG_ENABLE_STRONG_PULLUP_GPIO
    // An external pull-up circuit is used to supply extra current to OneWireBus devices
    // during temperature conversions.
    owb_use_strong_pullup_gpio(owb, CONFIG_STRONG_PULLUP_GPIO);
#endif
}

esp_err_t tempInitSensor(OneWireBus *bus, OneWireBus_ROMCode *code, DS18B20_Info *info){
    if(!tempVerify(bus,code)){
        return ESP_FAIL;
    }
    ds18b20_init(info,bus,*code);
    ds18b20_use_crc(info, true);  //Enable CRC on all reads
    if(ds18b20_set_resolution(info,DS_RESOLUTION)){
        return ESP_OK;
    }    
    return ESP_ERR_NOT_FOUND;
}

esp_err_t tempReadSensor(OneWireBus *bus, DS18B20_Info *info, float *temperature){
    if(!bus || !info){
        ESP_LOGE(OWBTAG,"Bus or info is NULL value!");
        return ESP_FAIL;
    }
    ds18b20_convert_all(bus);
    ds18b20_wait_for_conversion(info);
    if(ds18b20_read_temp(info,temperature) != DS18B20_OK){
        ESP_LOGW(OWBTAG,"DS18B20 Error");
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t tempBuildSensPtr(OneWireBus *bus, DS18B20_Info *sensors, unsigned short *sensseq){
    unsigned short bitseq = 0;
    esp_err_t errs[SENSORS_TOTAL] = {
        tempInitSensor(bus,&roomrom,sensors++),
        tempInitSensor(bus,&redrom,sensors++),
        tempInitSensor(bus,&greenrom,sensors++),
        tempInitSensor(bus,&bluerom,sensors++),
        tempInitSensor(bus,&whiterom,sensors++),
        tempInitSensor(bus,&yellowrom,sensors++),
        tempInitSensor(bus,&brownrom,sensors++),
        tempInitSensor(bus,&debugtemp1rom,sensors++),
        tempInitSensor(bus,&debugtemp2rom,sensors++)
    };
    
    for(uint8_t a = 0; a < SENSORS_TOTAL; a++){
        if(errs[a] == ESP_OK)
        bitseq |= 1 << a;
        else
        ESP_LOGW(OWBTAG,"OneWire Sensor \"%s\" konnte nicht gefunden werden!",sensornames[a]);
    }
    sensors -= SENSORS_TOTAL;
    *sensseq = bitseq;
    return ESP_OK;
}

esp_err_t tempReadAll(OneWireBus *bus, DS18B20_Info *sensors, float *temps, char *url, unsigned short sensseq){
    char varbuff[16] = ""; 
    strcpy(url,TEMP_URL);
    strcat(url,"?");
    ESP_LOGI(OWBTAG,"Sensseq is %d",sensseq);
    for(uint8_t a = 0; a < SENSORS_TOTAL; a++){
        strcat(url,sensornames[a]);
        strcat(url,"=");
        //printf("URL: %s\n",url);
        if(sensseq & (1 << a)){
            ESP_LOGI(OWBTAG,"Reading temperature of: %s",sensornames[a]);
            if(
                tempReadSensor(
                    bus,
                    sensors+a,
                    temps+a
                ) == ESP_OK
                && 
                *(temps+a) > ZERO_KELVIN
            ){
                sprintf(varbuff,"%.2f",*(temps+a));
                strcat(url,varbuff);
            }
        }else{
            ESP_LOGW(OWBTAG,"Sensor %s wasn't successfully initialized. ignoring device",sensornames[a]);
        }
        strcat(url,"&");
    }
    strcat(url,"solar=");
    if(tempAnalogPolynom(tempGetRt(),temps+SENSORS_TOTAL) == ESP_OK){
        sprintf(varbuff,"%.2f",*(temps+SENSORS_TOTAL));
        strcat(url,varbuff);
    }

    return ESP_OK;
}